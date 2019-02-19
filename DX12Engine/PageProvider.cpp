#include "PageProvider.h"
#include "VirtualTextureManager.h"
#include <d3d12.h>
#include "File.h"
#include "D3D12GraphicsEngine.h"
#include "VirtualFeedbackSubPass.h"

PageProvider::PageProvider() : freePageLoadRequestsCount{maxPagesLoading}
{
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

	freePageLoadRequests = nullptr;
	for (auto& request : pageLoadRequests)
	{
		request.nextPageLoadRequest = freePageLoadRequests;
		freePageLoadRequests = &request;
	}
}

//textures must be tiled
void PageProvider::addPageLoadRequestHelper(PageLoadRequest& streamingRequest, VirtualTextureManager& virtualTextureManager,
	void(*streamResource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
	void(*resourceUploaded)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources))
{
	VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID[streamingRequest.allocationInfo.textureLocation.textureId1()];
	streamingRequest.filename = resourceInfo.filename;
	std::size_t mipLevel = (std::size_t)streamingRequest.allocationInfo.textureLocation.mipLevel();
	auto width = resourceInfo.width;
	auto height = resourceInfo.height;
	uint64_t filePos = sizeof(DDSFileLoader::DdsHeaderDx12);
	for (std::size_t i = 0u; i != mipLevel; ++i)
	{
		std::size_t numBytes, rowBytes, numRows;
		DDSFileLoader::surfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
		filePos += numBytes;
		width = width >> 1u;
		if (width == 0u) width = 1u;
		height = height >> 1u;
		if (height == 0u) height = 1u;
	}
	std::size_t numBytes, rowBytes, numRows;
	DDSFileLoader::surfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
	constexpr std::size_t pageSize = 64u * 1024u;
	auto heightInPages = resourceInfo.heightInPages >> mipLevel;
	if (heightInPages == 0u) heightInPages = 1u;
	auto widthInPages = resourceInfo.widthInPages >> mipLevel;
	if (widthInPages == 0u) widthInPages = 1u;
	auto pageX = streamingRequest.allocationInfo.textureLocation.x();
	auto pageY = streamingRequest.allocationInfo.textureLocation.y();

	uint32_t pageHeightInTexels, pageWidthInTexels, pageWidthInBytes;
	DDSFileLoader::tileWidthAndHeightAndTileWidthInBytes(resourceInfo.format, pageWidthInTexels, pageHeightInTexels, pageWidthInBytes);

	filePos += pageY * rowBytes * pageHeightInTexels;
	if (pageY != (heightInPages - 1u))
	{
		filePos += pageSize * pageX;
		if (pageX < widthInPages)
		{
			streamingRequest.widthInBytes = pageWidthInBytes;
			streamingRequest.heightInTexels = pageHeightInTexels;
		}
		else
		{
			uint32_t lastColumnWidthInBytes = pageWidthInBytes - (widthInPages * pageWidthInBytes - (uint32_t)rowBytes);
			streamingRequest.widthInBytes = lastColumnWidthInBytes;
			streamingRequest.heightInTexels = pageHeightInTexels;
		}
	}
	else
	{
		uint32_t bottomPageHeight = pageHeightInTexels - (heightInPages * pageHeightInTexels - (uint32_t)numRows);
		uint32_t bottomPageSize = pageWidthInBytes * bottomPageHeight;
		filePos += bottomPageSize * pageX;
		if (pageX != (widthInPages - 1u))
		{
			streamingRequest.widthInBytes = pageWidthInBytes;
			streamingRequest.heightInTexels = bottomPageHeight;
		}
		else
		{
			uint32_t lastColumnWidthInBytes = pageWidthInBytes - (widthInPages * pageWidthInBytes - (uint32_t)rowBytes);
			streamingRequest.widthInBytes = lastColumnWidthInBytes;
			streamingRequest.heightInTexels = bottomPageHeight;
		}
	}
	
	streamingRequest.pageWidthInBytes = pageWidthInBytes;
	streamingRequest.file = resourceInfo.file;
	streamingRequest.resourceSize = streamingRequest.widthInBytes * streamingRequest.heightInTexels;
	streamingRequest.start = filePos;
	streamingRequest.end = filePos + streamingRequest.resourceSize;
	streamingRequest.deleteStreamingRequest = resourceUploaded;
	streamingRequest.streamResource = streamResource;
}

void PageProvider::addPageDataToResource(ID3D12Resource* resource, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, PageLoadRequest** pageLoadRequests, std::size_t pageCount,
	D3D12_TILE_REGION_SIZE& tileSize, PageCache& pageCache, ID3D12GraphicsCommandList* commandList, GpuCompletionEventManager<frameBufferCount>& gpuCompletionEventManager, uint32_t frameIndex,
	void(*uploadComplete)(void*, void*, void*))
{
	D3D12_RESOURCE_BARRIER barriers[1];
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[0].Transition.pResource = resource;
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1u, barriers);
	for (std::size_t i = 0u; i != pageCount; ++i)
	{
		commandList->CopyTiles(resource, &newPageCoordinates[i], &tileSize, pageLoadRequests[i]->uploadResource,
			pageLoadRequests[i]->uploadResourceOffset,
			D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
		gpuCompletionEventManager.addRequest(pageLoadRequests[i], uploadComplete, frameIndex);
		pageCache.setPageAsAllocated(pageLoadRequests[i]->allocationInfo.textureLocation, pageLoadRequests[i]->allocationInfo.heapLocation);
	}

	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	commandList->ResourceBarrier(1u, barriers);
}

void PageProvider::copyPageToUploadBuffer(StreamingManager::StreamingRequest* request, const unsigned char* data)
{
	PageLoadRequest& streamingRequest = *static_cast<PageLoadRequest*>(request);
	std::size_t widthInBytes = streamingRequest.widthInBytes;
	std::size_t heightInTexels = streamingRequest.heightInTexels;
	std::size_t pageWidthInBytes = streamingRequest.pageWidthInBytes;
	if (widthInBytes == pageWidthInBytes)
	{
		memcpy(streamingRequest.uploadBufferCurrentCpuAddress, data, streamingRequest.resourceSize);
	}
	else
	{
		const unsigned char* source = data;
		unsigned char* destination = streamingRequest.uploadBufferCurrentCpuAddress;
		for (std::size_t i = 0u; i != heightInTexels; ++i)
		{
			memcpy(destination, source, widthInBytes);
			source += widthInBytes;
			destination += pageWidthInBytes;
		}
	}
}

long long PageProvider::calculateMemoryBudgetInPages(IDXGIAdapter3* adapter)
{
	DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
	adapter->QueryVideoMemoryInfo(0u, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
	//total number of pages that can be used for non-pinned virtual texture pages
	long long maxPages = (long long)std::max((signed long long)0,
		((signed long long)videoMemoryInfo.Budget - (signed long long)videoMemoryInfo.CurrentUsage) / (64 * 1024) + (signed long long)pageAllocator.nonPinnedMemoryUsageInPages());
	return maxPages;
}

bool PageProvider::recalculateCacheSize(float& mipBias, float desiredMipBias, long long maxPages, std::size_t totalPagesNeeded, PageDeleter& pageDeleter)
{
	std::size_t pagesFullLowerBound = static_cast<std::size_t>(maxPages * memoryFullLowerBound);
	pagesFullLowerBound -= pagesFullLowerBound % PageAllocator::heapSizeInPages; //round down to heapSizeInPages
	const std::size_t oldCacheSize = pageCache.capacity();
	if (oldCacheSize < pagesFullLowerBound)
	{
		//we can allocate more memory
		if (mipBias > desiredMipBias)
		{
			//cache size needs increasing
			pageCache.increaseSize(pagesFullLowerBound);
			if (totalPagesNeeded * 5 < pagesFullLowerBound)
			{
				// mipBias needs decreasing
				--mipBias;
			}
		}
		else
		{
			std::size_t wantedPageCount = totalPagesNeeded * maxPageCountMultiplierLowerBound;
			if (wantedPageCount > oldCacheSize)
			{
				//cache size needs increasing
				auto newCacheSize = std::min(pagesFullLowerBound, wantedPageCount);
				pageCache.increaseSize(newCacheSize);
			}
		}
		return false;
	}
	std::size_t pagesFullUpperBound = static_cast<std::size_t>(maxPages * memoryFullUpperBound);
	pagesFullUpperBound -= pagesFullUpperBound % PageAllocator::heapSizeInPages;
	if(oldCacheSize > pagesFullUpperBound)
	{
		//memory usage must be reduced if posable
		pageAllocator.decreaseNonPinnedSize(pagesFullLowerBound, pageCache, pageDeleter);
		pageCache.decreaseSize(pagesFullLowerBound, pageDeleter);

		if(totalPagesNeeded > pagesFullUpperBound)
		{
			//need to increase mipBias
			++mipBias;
			return true;
		}
		return false;
	}
	
	//memory is in max range but not over it
	if(totalPagesNeeded > oldCacheSize)
	{
		//need to increase mipBias
		++mipBias;
		return true;
	}
	return false;
}

static inline void checkCacheForPage(std::pair<const TextureLocation, PageRequestData>& request, VirtualTextureInfoByID& texturesByID, PageCache& pageCache,
	ResizingArray<std::pair<TextureLocation, unsigned long long>>& posableLoadRequests)
{
	TextureLocation location = request.first;
	const VirtualTextureInfo& textureInfo = texturesByID[location.textureId1()];
	if(pageCache.getPage(location) != nullptr) return;
	const unsigned int nextMipLevel = (unsigned int)location.mipLevel() + 1u;
	if(nextMipLevel == textureInfo.lowestPinnedMip)
	{
		posableLoadRequests.push_back({location, request.second.count});
		return;
	}
	TextureLocation nextMipLocation;
	nextMipLocation.setX(location.x() >> 1u);
	nextMipLocation.setY(location.y() >> 1u);
	nextMipLocation.setMipLevel(nextMipLevel);
	nextMipLocation.setTextureId1(location.textureId1());
	nextMipLocation.setTextureId2(255u);
	nextMipLocation.setTextureId3(255u);
	if(pageCache.getPage(nextMipLocation) != nullptr)
	{
		posableLoadRequests.push_back({location, request.second.count});
	}
}

void PageProvider::checkCacheForPages(decltype(uniqueRequests)& pageRequests, VirtualTextureInfoByID& texturesByID, PageCache& pageCache, 
	ResizingArray<std::pair<TextureLocation, unsigned long long>>& posableLoadRequests)
{
	for(auto& request : pageRequests)
	{
		checkCacheForPage(request, texturesByID, pageCache, posableLoadRequests);
	}
}

void PageProvider::addNewPagesToResources(PageProvider& pageProvider, D3D12GraphicsEngine& graphicsEngine, VirtualTextureInfoByID& texturesByID,
	GpuCompletionEventManager<frameBufferCount>& gpuCompletionEventManager, ID3D12GraphicsCommandList* commandList,
	void(*uploadComplete)(void*, void*, void*))
{
	ID3D12CommandQueue* commandQueue = graphicsEngine.directCommandQueue;
	ID3D12Device* graphicsDevice = graphicsEngine.graphicsDevice;
	PageAllocator& pageAllocator = pageProvider.pageAllocator;
	PageCache& pageCache = pageProvider.pageCache;
	auto frameIndex = graphicsEngine.frameIndex;

	PageLoadRequest* halfFinishedPageRequests = pageProvider.halfFinishedPageLoadRequests.exchange(nullptr, std::memory_order::memory_order_acquire);

	if(halfFinishedPageRequests == nullptr) return;

	//add all remaining new pages to the cache and drop old pages from the cache
	std::size_t newPageCount = 0;
	PageLoadRequest* newPages[maxPagesLoading];
	do
	{
		if(pageCache.contains(halfFinishedPageRequests->allocationInfo.textureLocation))
		{
			newPages[newPageCount] = halfFinishedPageRequests;
			++newPageCount;
		}
		halfFinishedPageRequests = halfFinishedPageRequests->nextPageLoadRequest;
	} while(halfFinishedPageRequests != nullptr);

	if(newPageCount == 0u) return;

	//sort in order of resource
	std::sort(newPages, newPages + newPageCount, [](PageLoadRequest* first, PageLoadRequest* second)
	{
		return first->allocationInfo.textureLocation.textureId1() < second->allocationInfo.textureLocation.textureId1();
	});

	D3D12_TILE_REGION_SIZE tileSize;
	tileSize.Depth = 1u;
	tileSize.Height = 1u;
	tileSize.Width = 1u;
	tileSize.NumTiles = 1u;
	tileSize.UseBox = TRUE;
	D3D12_TILED_RESOURCE_COORDINATE newPageCoordinates[maxPagesLoading];

	ID3D12Resource* previousResource = texturesByID[newPages[0u]->allocationInfo.textureLocation.textureId1()].resource;
	std::size_t lastIndex = 0u;
	std::size_t i = 0u;
	for(; i != newPageCount; ++i)
	{
		ID3D12Resource* resource = texturesByID[newPages[i]->allocationInfo.textureLocation.textureId1()].resource;
		if(resource != previousResource)
		{
			const std::size_t pageCount = i - lastIndex;
			pageAllocator.addPages(newPageCoordinates + lastIndex, pageCount, previousResource, commandQueue, graphicsDevice, HeapLocationsIterator{newPages + lastIndex});
			pageProvider.addPageDataToResource(previousResource, newPageCoordinates + lastIndex, newPages + lastIndex, pageCount, tileSize, pageCache,
				commandList, gpuCompletionEventManager, frameIndex, uploadComplete);
			lastIndex = i;
			previousResource = resource;
		}

		newPageCoordinates[i].X = (UINT)newPages[i]->allocationInfo.textureLocation.x();
		newPageCoordinates[i].Y = (UINT)newPages[i]->allocationInfo.textureLocation.y();
		newPageCoordinates[i].Z = (UINT)0u;
		newPageCoordinates[i].Subresource = (UINT)newPages[i]->allocationInfo.textureLocation.mipLevel();
	}
	const std::size_t pageCount = i - lastIndex;
	pageAllocator.addPages(newPageCoordinates + lastIndex, pageCount, previousResource, commandQueue, graphicsDevice, HeapLocationsIterator{newPages + lastIndex});
	pageProvider.addPageDataToResource(previousResource, newPageCoordinates + lastIndex, newPages + lastIndex, pageCount, tileSize, pageCache,
		commandList, gpuCompletionEventManager, frameIndex, uploadComplete);
}

void PageProvider::shrinkNumberOfLoadRequestsIfNeeded(std::size_t numTexturePagesThatCanBeRequested)
{
	if(numTexturePagesThatCanBeRequested < posableLoadRequests.size())
	{
		if(numTexturePagesThatCanBeRequested != 0u)
		{
			std::nth_element(posableLoadRequests.begin(), posableLoadRequests.begin() + numTexturePagesThatCanBeRequested, posableLoadRequests.end(), [](const auto& lhs, const auto& rhs)
			{
				return lhs.second > rhs.second; //sort in descending order of area on screen
			});
		}
		//reduce posableLoadRequests size to numTexturesThatCanBeRequested
		posableLoadRequests.resize(numTexturePagesThatCanBeRequested);
	}
}

void PageProvider::collectReturnedPageLoadRequests()
{
	auto oldReturnedPageLoadRequests = returnedPageLoadRequests.exchange(nullptr, std::memory_order::memory_order_acquire);
	if(oldReturnedPageLoadRequests != nullptr)
	{
		auto oldFreePageLoadRequests = freePageLoadRequests;
		freePageLoadRequests = oldReturnedPageLoadRequests;
		while(true)
		{
			++freePageLoadRequestsCount;
			if(oldReturnedPageLoadRequests->nextPageLoadRequest == nullptr)
			{
				break;
			}
			else
			{
				oldReturnedPageLoadRequests = oldReturnedPageLoadRequests->nextPageLoadRequest;
			}
		}
		oldReturnedPageLoadRequests->nextPageLoadRequest = oldFreePageLoadRequests;
	}
}

template<class HashMap>
static inline void requestMipLevels(unsigned int mipLevel, const unsigned int lowestPinnedMip, TextureLocation feedbackData, HashMap& uniqueRequests)
{
	if(mipLevel >= lowestPinnedMip) return;
	auto x = feedbackData.x() >> mipLevel;
	auto y = feedbackData.y() >> mipLevel;
	while(true)
	{
		feedbackData.setX(x);
		feedbackData.setY(y);
		unsigned long long& count = uniqueRequests[feedbackData].count;
		++count;
		++mipLevel;
		if(count != 1u || mipLevel == lowestPinnedMip) return;
		feedbackData.setMipLevel(mipLevel);
		x >>= 1u;
		y >>= 1u;
	}
}

void PageProvider::gatherPageRequests(unsigned char* feadBackBuffer, unsigned long sizeInBytes, VirtualTextureInfoByID& texturesByID)
{
	auto& requests = this->uniqueRequests;
	const auto feadBackBufferEnd = feadBackBuffer + sizeInBytes;
	for(; feadBackBuffer != feadBackBufferEnd; feadBackBuffer += 8u)
	{
		TextureLocation feedbackData{*reinterpret_cast<uint64_t*>(feadBackBuffer)};

		unsigned int nextMipLevel = (unsigned int)feedbackData.mipLevel();
		auto textureId = feedbackData.textureId1();
		auto texture2d = feedbackData.textureId2();
		auto texture3d = feedbackData.textureId3();

		feedbackData.setTextureId2(0);
		feedbackData.setTextureId3(0);

		if(textureId != 255u)
		{
			VirtualTextureInfo& textureInfo = texturesByID[textureId];
			requestMipLevels(nextMipLevel, textureInfo.lowestPinnedMip, feedbackData, requests);
		}
		if(texture2d != 255u)
		{
			feedbackData.setTextureId1(texture2d);
			VirtualTextureInfo& textureInfo = texturesByID[texture2d];
			requestMipLevels(nextMipLevel, textureInfo.lowestPinnedMip, feedbackData, requests);
		}
		if(texture3d != 255u)
		{
			feedbackData.setTextureId1(texture3d);
			VirtualTextureInfo& textureInfo = texturesByID[texture3d];
			requestMipLevels(nextMipLevel, textureInfo.lowestPinnedMip, feedbackData, requests);
		}
	}
}

void PageProvider::processPageRequestsHelper(IDXGIAdapter3* adapter, VirtualTextureInfoByID& texturesByID, float& mipBias, float desiredMipBias, PageDeleter& pageDeleter)
{
	//Work out memory budget, grow or shrink cache as required and change mip bias as required
	long long memoryBedgetInPages = calculateMemoryBudgetInPages(adapter);
	while(recalculateCacheSize(mipBias, desiredMipBias, memoryBedgetInPages, uniqueRequests.size(), pageDeleter))
	{
		increaseMipBias(uniqueRequests, texturesByID);
	}

	//work out which pages are in the cache and which pages need loading
	checkCacheForPages(uniqueRequests, texturesByID, pageCache, posableLoadRequests);
	uniqueRequests.clear();

	collectReturnedPageLoadRequests();
	shrinkNumberOfLoadRequestsIfNeeded(std::min(freePageLoadRequestsCount, pageCache.capacity() - pageCache.size()));
}

void PageProvider::increaseMipBias(decltype(uniqueRequests)& pageRequests, VirtualTextureInfoByID& texturesByID)
{
	for(auto& pageRequest : pageRequests)
	{
		VirtualTextureInfo& textureInfo = texturesByID[pageRequest.first.textureId1()];
		const auto newMipLevel = pageRequest.first.mipLevel() + 1u;
		if(textureInfo.lowestPinnedMip == newMipLevel) continue;
		TextureLocation newLocation = pageRequest.first;
		newLocation.setMipLevel(newMipLevel);
		posableLoadRequests.push_back({newLocation, pageRequest.second.count});
	}
	pageRequests.clear();
	for(auto& pageRequest : posableLoadRequests)
	{
		pageRequests[pageRequest.first].count += pageRequest.second;
	}
	posableLoadRequests.clear();
}