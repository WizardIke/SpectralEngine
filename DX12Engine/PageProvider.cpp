#include "PageProvider.h"
#include "VirtualTextureManager.h"
#include <d3d12.h>
#include <d3d12.h>
#include "File.h"
#include "D3D12GraphicsEngine.h"

PageProvider::PageProvider(float desiredMipBias, IDXGIAdapter3* adapter, ID3D12Device* graphicsDevice)
{
	DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
	auto result = adapter->QueryVideoMemoryInfo(0u, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
	assert(result == S_OK);
	maxPages = ((signed long long)videoMemoryInfo.Budget - (signed long long)videoMemoryInfo.CurrentUsage) / (64 * 1024);
	memoryUsage = videoMemoryInfo.CurrentUsage;

	mipBias = desiredMipBias;
	this->desiredMipBias = desiredMipBias;

	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 0u;
	uploadHeapProperties.VisibleNodeMask = 0u;

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	resourceDesc.Width = 64 * 1024 * maxPagesLoading;
	resourceDesc.Height = 1u;

	for (auto& request : pageLoadRequests)
	{
		request.state.store(PageLoadRequest::State::unused, std::memory_order::memory_order_relaxed);
	}
}

//textures must be tiled
void PageProvider::addPageLoadRequestHelper(PageLoadRequest& pageRequest, VirtualTextureManager& virtualTextureManager, StreamingManager& streamingManager,
	void(*useSubresource)(void* executor, void* sharedResources, HalfFinishedUploadRequest& useSubresourceRequest))
{
	VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID[pageRequest.allocationInfo.textureLocation.textureId1()];
	pageRequest.filename = resourceInfo.filename;
	size_t mipLevel = (size_t)pageRequest.allocationInfo.textureLocation.mipLevel();
	auto width = resourceInfo.width;
	auto height = resourceInfo.height;
	uint64_t filePos = sizeof(DDSFileLoader::DdsHeaderDx12);
	for (size_t i = 0u; i != mipLevel; ++i)
	{
		size_t numBytes, rowBytes, numRows;
		DDSFileLoader::surfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
		filePos += numBytes;
		width = width >> 1u;
		if (width == 0u) width = 1u;
		height = height >> 1u;
		if (height == 0u) height = 1u;
	}
	size_t numBytes, rowBytes, numRows;
	DDSFileLoader::surfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
	constexpr size_t pageSize = 64u * 1024u;
	auto heightInPages = resourceInfo.heightInPages >> mipLevel;
	if (heightInPages == 0u) heightInPages = 1u;
	auto widthInPages = resourceInfo.widthInPages >> mipLevel;
	if (widthInPages == 0u) widthInPages = 1u;
	auto pageX = pageRequest.allocationInfo.textureLocation.x();
	auto pageY = pageRequest.allocationInfo.textureLocation.y();
	
	RamToVramUploadRequest request;

	uint32_t pageHeightInTexels, pageWidthInTexels, pageWidthInBytes;
	DDSFileLoader::tileWidthAndHeightAndTileWidthInBytes(resourceInfo.format, pageWidthInTexels, pageHeightInTexels, pageWidthInBytes);

	filePos += pageY * rowBytes * pageHeightInTexels;
	if (pageY != (heightInPages - 1u))
	{
		filePos += pageSize * pageX;
		if (pageX < widthInPages)
		{
			streamingManager.addUploadRequest(request);
			request.meshInfo.indicesSize = pageWidthInBytes;
			request.meshInfo.verticesSize = pageHeightInTexels;
		}
		else
		{
			uint32_t lastColumnWidthInBytes = pageWidthInBytes - (widthInPages * pageWidthInBytes - (uint32_t)rowBytes);
			request.meshInfo.indicesSize = lastColumnWidthInBytes;
			request.meshInfo.verticesSize = pageHeightInTexels;
		}
	}
	else
	{
		uint32_t bottomPageHeight = pageHeightInTexels - (heightInPages * pageHeightInTexels - (uint32_t)numRows);
		uint32_t bottomPageSize = pageWidthInBytes * bottomPageHeight;
		filePos += bottomPageSize * pageX;
		if (pageX != (widthInPages - 1u))
		{
			request.meshInfo.indicesSize = pageWidthInBytes;
			request.meshInfo.verticesSize = bottomPageHeight;
		}
		else
		{
			uint32_t lastColumnWidthInBytes = pageWidthInBytes - (widthInPages * pageWidthInBytes - (uint32_t)rowBytes);
			request.meshInfo.indicesSize = lastColumnWidthInBytes;
			request.meshInfo.verticesSize = bottomPageHeight;
		}
	}

	pageRequest.offsetInFile = filePos;
	
	request.meshInfo.padding = pageWidthInBytes;
	request.arraySize = 1u;
	request.currentArrayIndex = 0u;
	request.currentMipLevel = 0u;
	request.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	request.file = resourceInfo.file;
	request.meshInfo.sizeInBytes = pageSize;
	request.mostDetailedMip = 0u;
	request.requester = &pageRequest;

	request.resourceUploadedPointer = [](void* requester, void* executor, void* sharedResources)
	{
		PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(requester);
		pageRequest.state.store(PageProvider::PageLoadRequest::State::finished, std::memory_order::memory_order_release);
	};

	request.useSubresource = useSubresource;
}

void PageProvider::addPageDataToResource(ID3D12Resource* resource, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, size_t pageCount,
	D3D12_TILE_REGION_SIZE& tileSize, unsigned long* uploadBufferOffsets, ID3D12GraphicsCommandList* commandList, GpuCompletionEventManager& gpuCompletionEventManager, uint32_t frameIndex)
{
	D3D12_RESOURCE_BARRIER barriers[1];
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[0].Transition.pResource = resource;
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1u, barriers);
	for (size_t i = 0u; i != pageCount; ++i)
	{
		commandList->CopyTiles(resource, &newPageCoordinates[i], &tileSize, pageLoadRequests[uploadBufferOffsets[i]].subresourceRequest->uploadRequest->uploadResource,
			pageLoadRequests[uploadBufferOffsets[i]].subresourceRequest->uploadResourceOffset,
			D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);

		gpuCompletionEventManager.addRequest(&pageLoadRequests[uploadBufferOffsets[i]], [](void* requester, void* exe, void* sr)
		{
			PageLoadRequest* request = reinterpret_cast<PageLoadRequest*>(requester);
			request->state.store(PageLoadRequest::State::unused, std::memory_order::memory_order_release);
			request->subresourceRequest->copyFenceValue.store(0u, std::memory_order::memory_order_release); //copy has finished.
		}, frameIndex);
	}

	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	commandList->ResourceBarrier(1u, barriers);
}

bool PageProvider::NewPageIterator::NewPage::operator<(const NewPage& other)
{
	VirtualTextureInfo* resourceInfo = &virtualTextureManager.texturesByID[page->textureLocation.textureId1()];
	VirtualTextureInfo* otherResourceInfo = &virtualTextureManager.texturesByID[other.page->textureLocation.textureId1()];
	return resourceInfo < otherResourceInfo;
}

void PageProvider::copyPageToUploadBuffer(HalfFinishedUploadRequest& useSubresourceRequest, const uint8_t* data)
{
	auto& uploadRequest = *useSubresourceRequest.uploadRequest;
	size_t widthInBytes = uploadRequest.meshInfo.indicesSize;
	size_t heightInTexels = uploadRequest.meshInfo.verticesSize;
	size_t pageWidthInBytes = (size_t)uploadRequest.meshInfo.padding;
	if (widthInBytes == pageWidthInBytes)
	{
		memcpy(useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos, data, widthInBytes * heightInTexels);
	}
	else
	{
		const uint8_t* source = data;
		uint8_t* destination = (uint8_t*)useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos;
		for (std::size_t i = 0u; i != heightInTexels; ++i)
		{
			memcpy(destination, source, widthInBytes);
			source += widthInBytes;
			destination += pageWidthInBytes;
		}
	}

	PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(uploadRequest.requester);
	pageRequest.subresourceRequest = &useSubresourceRequest;
}

unsigned int PageProvider::recalculateCacheSize(IDXGIAdapter3* adapter, size_t totalPagesNeeded)
{
	DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
	adapter->QueryVideoMemoryInfo(0u, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
	maxPages += ((signed long long)memoryUsage - (signed long long)videoMemoryInfo.CurrentUsage) / (64 * 1024);
	size_t maxPagesMinZero = maxPages > 0 ? (size_t)maxPages : 0u;
	memoryUsage = videoMemoryInfo.CurrentUsage;

	newCacheSize = pageCache.capacity();
	unsigned int mipBiasIncrease = 0u;
	size_t pagesFullLowerBound = static_cast<size_t>(maxPagesMinZero * memoryFullLowerBound);
	pagesFullLowerBound -= pagesFullLowerBound % PageAllocator::heapSizeInPages;
	if (pageCache.capacity() < pagesFullLowerBound)
	{
		//we can allocate more memory
		if (mipBias > desiredMipBias)
		{
			//cache size needs increasing
			newCacheSize = static_cast<size_t>(maxPagesMinZero * ((memoryFullLowerBound + memoryFullUpperBound) / 2u));
			pageCache.increaseSize(newCacheSize);
			if (totalPagesNeeded * 4 < pageCache.capacity())
			{
				// mipBias needs decreasing
				--mipBias;
			}
		}
		else
		{
			if (totalPagesNeeded * maxPageCountMultiplierLowerBound > pageCache.capacity())
			{
				//cache size needs increasing
				size_t maxPagesUsable = size_t(maxPagesMinZero * ((memoryFullLowerBound + memoryFullUpperBound) / 2u));
				maxPagesUsable -= maxPagesUsable % PageAllocator::heapSizeInPages;
				newCacheSize = std::min(maxPagesUsable, totalPagesNeeded * ((maxPageCountMultiplierLowerBound + maxPageCountMultiplierUpperBound) / 2u));
				pageCache.increaseSize(newCacheSize);
			}
			else if (totalPagesNeeded * maxPageCountMultiplierUpperBound < pageCache.capacity())
			{
				//we have more memory than we need, decrease cache size
				newCacheSize = totalPagesNeeded * ((maxPageCountMultiplierLowerBound + maxPageCountMultiplierUpperBound) / 2u);
			}
		}
	}
	else
	{
		size_t pagesFullUpperBound = static_cast<size_t>(maxPagesMinZero * memoryFullUpperBound);
		pagesFullUpperBound -= pagesFullUpperBound % PageAllocator::heapSizeInPages;
		if (pageCache.capacity() > pagesFullUpperBound)
		{
			//memory usage must be reduced if posable
			if (totalPagesNeeded > pagesFullUpperBound)
			{
				//need to increase mipBias
				++mipBias;
				mipBiasIncrease = 1u;
				totalPagesNeeded /= 4u; //mgiht not reduce by exactly the right amount but should be close enough
			}
			//reduce cache size
			newCacheSize = size_t(maxPagesMinZero * ((memoryFullLowerBound + memoryFullUpperBound) / 2u));
			newCacheSize -= newCacheSize % PageAllocator::heapSizeInPages;
		}
		else
		{
			//memory is in max range but not over it
			if (totalPagesNeeded * maxPageCountMultiplierUpperBound < pageCache.capacity())
			{
				//we have more memory than we need, decrease cache size
				newCacheSize = totalPagesNeeded * ((maxPageCountMultiplierLowerBound + maxPageCountMultiplierUpperBound) / 2u);
			}
			else if (totalPagesNeeded > pageCache.capacity())
			{
				//need to increase mipBias
				++mipBias;
				mipBiasIncrease = 1u;
				totalPagesNeeded /= 4u; //mgiht not reduce by exactly the right amount but should be close enough
			}
		}
	}
	return mipBiasIncrease;
}

void PageProvider::workoutIfPageNeededInCache(unsigned int mipBiasIncrease, std::pair<const textureLocation, PageRequestData>& request, VirtualTextureManager& virtualTextureManager, size_t& numRequiredPagesInCache)
{
	textureLocation location;
	location.value = request.first.value;
	auto textureId = request.first.textureId1();
	const unsigned int mipLevel = static_cast<unsigned int>(request.first.mipLevel()) + mipBiasIncrease;
	location.setMipLevel(mipLevel);
	auto newX = location.x() >> mipBiasIncrease;
	auto newY = location.y() >> mipBiasIncrease;
	location.setX(newX);
	location.setY(newY);
	const VirtualTextureInfo& textureInfo = virtualTextureManager.texturesByID[textureId];

	if (pageCache.getPage(location) != nullptr)
	{
		++numRequiredPagesInCache;
	}
	else
	{
		const unsigned int nextMipLevel = mipLevel + 1u;
		textureLocation nextMipLocation;
		nextMipLocation.setX(newX >> 1u);
		nextMipLocation.setY(newY >> 1u);
		nextMipLocation.setMipLevel(nextMipLevel);
		nextMipLocation.setTextureId1(textureId);
		nextMipLocation.setTextureId2(255u);
		nextMipLocation.setTextureId3(255u);
		if (nextMipLevel == textureInfo.lowestPinnedMip || pageCache.getPage(nextMipLocation) != nullptr)
		{
			bool pageAlreadyLoading = false;
			for (const auto& pageRequest : pageLoadRequests)
			{
				auto state = pageRequest.state.load(std::memory_order::memory_order_acquire);
				if (state != PageLoadRequest::State::unused)
				{
					if (pageRequest.allocationInfo.textureLocation == location)
					{
						pageAlreadyLoading = true;
						break;
					}
				}
			}

			if (!pageAlreadyLoading)
			{
				posableLoadRequests.push_back({ location, request.second.count });
			}
		}
	}
}

size_t PageProvider::findLoadedTexturePages()
{
	newPageCount = 0u;
	size_t numTexturesThatCanBeRequested = 0u;
	for (auto& pageRequest : pageLoadRequests)
	{
		auto state = pageRequest.state.load(std::memory_order::memory_order_acquire);
		if (state == PageLoadRequest::State::finished)
		{
			newPages[newPageCount] = pageRequest.allocationInfo;
			newPageOffsetsInLoadRequests[newPageCount] = static_cast<unsigned long>(&pageRequest - pageLoadRequests);
			++newPageCount;
		}
		else if (state == PageLoadRequest::State::unused)
		{
			++numTexturesThatCanBeRequested;
		}
	}
	return numTexturesThatCanBeRequested;
}

void PageProvider::addNewPagesToResources(PageProvider& pageProvider, D3D12GraphicsEngine& graphicsEngine, VirtualTextureManager& virtualTextureManager, GpuCompletionEventManager& gpuCompletionEventManager, ID3D12GraphicsCommandList* commandList)
{
	ID3D12CommandQueue* commandQueue = graphicsEngine.directCommandQueue;
	ID3D12Device* graphicsDevice = graphicsEngine.graphicsDevice;
	PageAllocator& pageAllocator = pageProvider.pageAllocator;
	PageCache& pageCache = pageProvider.pageCache;
	PageDeleter<decltype(virtualTextureManager.texturesByID)> pageDeleter(pageAllocator, virtualTextureManager.texturesByID, commandQueue);
	size_t newCacheSize = pageProvider.newCacheSize;
	size_t newPageCount = pageProvider.newPageCount;
	PageAllocationInfo* newPages = pageProvider.newPages;
	unsigned long* newPageOffsetsInLoadRequests = pageProvider.newPageOffsetsInLoadRequests;
	auto frameIndex = graphicsEngine.frameIndex;
	if (newCacheSize < pageCache.capacity())
	{
		pageAllocator.decreaseNonPinnedSize(newCacheSize, pageCache, commandQueue, virtualTextureManager.texturesByID);
		pageCache.decreaseSize(newCacheSize, pageDeleter);
	}
	//add all remaining new pages to the cache and drop old pages from the cache
	if (newPageCount != 0u)
	{
		D3D12_TILE_REGION_SIZE tileSize;
		tileSize.Depth = 1u;
		tileSize.Height = 1u;
		tileSize.Width = 1u;
		tileSize.NumTiles = 1u;
		tileSize.UseBox = TRUE;
		D3D12_TILED_RESOURCE_COORDINATE newPageCoordinates[maxPagesLoading];

		//sort in order of resource
		std::sort(NewPageIterator{ newPages, newPageOffsetsInLoadRequests, virtualTextureManager },
			NewPageIterator{ newPages + newPageCount, newPageOffsetsInLoadRequests + newPageCount, virtualTextureManager });

		VirtualTextureInfo* previousResourceInfo = &virtualTextureManager.texturesByID[newPages[0u].textureLocation.textureId1()];
		size_t lastIndex = 0u;
		size_t i = 0u;
		for (; i != newPageCount; ++i)
		{
			newPageCoordinates[i].X = (UINT)newPages[i].textureLocation.x();
			newPageCoordinates[i].Y = (UINT)newPages[i].textureLocation.y();
			newPageCoordinates[i].Z = (UINT)0u;
			newPageCoordinates[i].Subresource = (UINT)newPages[i].textureLocation.mipLevel();

			VirtualTextureInfo* resourceInfo = &virtualTextureManager.texturesByID[newPages[i].textureLocation.textureId1()];
			if (resourceInfo != previousResourceInfo)
			{
				const size_t pageCount = i - lastIndex;
				pageAllocator.addPages(newPageCoordinates + lastIndex, pageCount, *previousResourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);
				pageProvider.addPageDataToResource(previousResourceInfo->resource, &newPageCoordinates[lastIndex], pageCount, tileSize, &newPageOffsetsInLoadRequests[lastIndex],
					commandList, gpuCompletionEventManager, frameIndex);
				lastIndex = i;
				previousResourceInfo = resourceInfo;
			}
		}
		const size_t pageCount = i - lastIndex;
		pageAllocator.addPages(newPageCoordinates + lastIndex, pageCount, *previousResourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);
		pageProvider.addPageDataToResource(previousResourceInfo->resource, &newPageCoordinates[lastIndex], pageCount, tileSize, &newPageOffsetsInLoadRequests[lastIndex],
			commandList, gpuCompletionEventManager, frameIndex);
		pageCache.addPages(newPages, newPageCount, pageDeleter);
	}
	pageDeleter.finish();
}