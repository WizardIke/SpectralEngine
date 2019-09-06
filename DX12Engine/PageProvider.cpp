#include "PageProvider.h"
#include "VirtualTextureManager.h"
#include <d3d12.h>
#include "File.h"
#include "VirtualFeedbackSubPass.h"

PageProvider::PageProvider(VirtualFeedbackSubPass& feedbackAnalizerSubPass1, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager) :
	freePageLoadRequestsCount{maxPagesLoading},
	feedbackAnalizerSubPass(feedbackAnalizerSubPass1),
	streamingManager(streamingManager),
	graphicsEngine(graphicsEngine),
	asynchronousFileManager(asynchronousFileManager)
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
		request.pageProvider = this;
		static_cast<LinkedTask&>(request).next = static_cast<LinkedTask*>(freePageLoadRequests);
		freePageLoadRequests = &request;
	}
}

//textures must be tiled
void PageProvider::addPageLoadRequestHelper(PageLoadRequest& streamingRequest,
	void(*streamResource)(StreamingManager::StreamingRequest* request, void* tr),
	void(*resourceUploaded)(StreamingManager::StreamingRequest* request, void* tr))
{
	VirtualTextureInfoByID& texturesByID = streamingRequest.pageProvider->texturesByID;
	VirtualTextureInfo& resourceInfo = texturesByID[streamingRequest.allocationInfo.textureLocation.textureId];
	streamingRequest.filename = resourceInfo.filename;
	unsigned int mipLevel = streamingRequest.allocationInfo.textureLocation.mipLevel;
	auto width = resourceInfo.width;
	auto height = resourceInfo.height;
	uint64_t filePos = sizeof(DDSFileLoader::DdsHeaderDx12);
	for (unsigned int i = 0u; i != mipLevel; ++i)
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
	auto pageX = streamingRequest.allocationInfo.textureLocation.x;
	auto pageY = streamingRequest.allocationInfo.textureLocation.y;

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

void PageProvider::addPageDataToResource(VirtualTextureInfo& textureInfo, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, PageLoadRequest** pageLoadRequests, std::size_t pageCount,
	D3D12_TILE_REGION_SIZE& tileSize, PageCache& pageCache, ID3D12GraphicsCommandList* commandList, GraphicsEngine& graphicsEngine,
	void(*uploadComplete)(PrimaryTaskFromOtherThreadQueue::Task& task, void* tr))
{
	ID3D12Resource* const resource = textureInfo.resource;

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
		pageLoadRequests[i]->execute = uploadComplete;
		graphicsEngine.executeWhenGpuFinishesCurrentFrame(*pageLoadRequests[i]);
		pageCache.setPageAsAllocated(pageLoadRequests[i]->allocationInfo.textureLocation, textureInfo, pageLoadRequests[i]->allocationInfo.heapLocation);
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

PageProvider::NewCacheCapacity PageProvider::recalculateCacheSize(float& mipBias, float desiredMipBias, long long maxPages, std::size_t totalPagesNeeded, std::size_t oldCacheCapacity)
{
	std::size_t pagesFullLowerBound = static_cast<std::size_t>(maxPages * memoryFullLowerBound);
	pagesFullLowerBound -= pagesFullLowerBound % PageAllocator::heapSizeInPages; //round down to heapSizeInPages
	if (oldCacheCapacity < pagesFullLowerBound)
	{
		std::size_t newCachCapacity;
		//we can allocate more memory
		if (mipBias > desiredMipBias)
		{
			//cache size needs increasing
			newCachCapacity = pagesFullLowerBound;
			if (totalPagesNeeded * 5 < pagesFullLowerBound)
			{
				// mipBias needs decreasing
				--mipBias;
			}
		}
		else
		{
			std::size_t wantedPageCount = totalPagesNeeded * maxPageCountMultiplierLowerBound;
			if (wantedPageCount > oldCacheCapacity)
			{
				//cache size needs increasing
				newCachCapacity = std::min(pagesFullLowerBound, wantedPageCount);
			}
			else
			{
				newCachCapacity = oldCacheCapacity;
			}
		}
		return { false, newCachCapacity };
	}
	std::size_t pagesFullUpperBound = static_cast<std::size_t>(maxPages * memoryFullUpperBound);
	pagesFullUpperBound -= pagesFullUpperBound % PageAllocator::heapSizeInPages;
	if(oldCacheCapacity > pagesFullUpperBound)
	{
		//memory usage must be reduced if posable
		if(totalPagesNeeded > pagesFullUpperBound)
		{
			//need to increase mipBias
			++mipBias;
			return { true, pagesFullLowerBound };
		}
		return { false, pagesFullLowerBound };
	}
	
	//memory is in max range but not over it
	if(totalPagesNeeded > oldCacheCapacity)
	{
		//need to increase mipBias
		++mipBias;
		return { true, oldCacheCapacity };
	}
	return { false, oldCacheCapacity };
}

void PageProvider::checkCacheForPage(std::pair<const PageResourceLocation, PageRequestData>& request, VirtualTextureInfoByID& texturesByID, PageCache& pageCache,
	ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests)
{
	PageResourceLocation location = request.first;
	VirtualTextureInfo& textureInfo = texturesByID[location.textureId];
	if (pageCache.contains(location, textureInfo))
	{
		return;
	}
	const unsigned char nextMipLevel = location.mipLevel + 1u;
	if(nextMipLevel == textureInfo.lowestPinnedMip)
	{
		posableLoadRequests.push_back({location, request.second.count});
		return;
	}
	PageResourceLocation nextMipLocation;
	nextMipLocation.x = location.x >> 1u;
	nextMipLocation.y = location.y >> 1u;
	nextMipLocation.mipLevel = nextMipLevel;
	nextMipLocation.textureId = location.textureId;
	if(pageCache.contains(nextMipLocation, textureInfo))
	{
		posableLoadRequests.push_back({location, request.second.count});
	}
}

void PageProvider::checkCacheForPages(decltype(uniqueRequests)& pageRequests, VirtualTextureInfoByID& texturesByID, PageCache& pageCache, 
	ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests)
{
	for(auto& request : pageRequests)
	{
		checkCacheForPage(request, texturesByID, pageCache, posableLoadRequests);
	}
}

void PageProvider::addNewPagesToResources(ID3D12GraphicsCommandList* commandList, void(*uploadComplete)(LinkedTask& task, void* tr), void* tr)
{
	ID3D12CommandQueue* commandQueue = graphicsEngine.directCommandQueue;
	ID3D12Device* graphicsDevice = graphicsEngine.graphicsDevice;
	PageAllocator& allocator = pageAllocator;
	PageCache& cache = pageCache;

	PageLoadRequest* halfFinishedPageRequests = static_cast<PageLoadRequest*>(static_cast<LinkedTask*>(halfFinishedPageLoadRequests.popAll()));

	if(halfFinishedPageRequests == nullptr) return;

	//add all remaining new pages to the cache
	std::size_t newPageCount = 0;
	PageLoadRequest* newPages[maxPagesLoading];
	do
	{
		auto textureLocation = halfFinishedPageRequests->allocationInfo.textureLocation;
		VirtualTextureInfo& textureInfo = texturesByID[textureLocation.textureId];
		if(cache.containsDoNotMarkAsRecentlyUsed(halfFinishedPageRequests->allocationInfo.textureLocation, textureInfo))
		{
			newPages[newPageCount] = halfFinishedPageRequests;
			++newPageCount;
		}
		else
		{
			--textureInfo.pageCacheData.numberOfUnneededLoadingPages;
			if (textureInfo.pageCacheData.numberOfUnneededLoadingPages == 0u && textureInfo.unloadRequest != nullptr)
			{
				assert(textureInfo.textureID != 255);
				texturesByID.deallocate(textureInfo.textureID);
				textureInfo.unloadRequest->callback(*textureInfo.unloadRequest, tr);
			}
		}
		halfFinishedPageRequests = static_cast<PageLoadRequest*>(static_cast<LinkedTask*>(static_cast<LinkedTask*>(halfFinishedPageRequests)->next));
	} while(halfFinishedPageRequests != nullptr);

	if(newPageCount == 0u) return;

	//group pages from the same resource together. 
	std::sort(newPages, newPages + newPageCount, [](PageLoadRequest* first, PageLoadRequest* second)
	{
		return first->allocationInfo.textureLocation.textureId < second->allocationInfo.textureLocation.textureId;
	});

	D3D12_TILE_REGION_SIZE tileSize;
	tileSize.Depth = 1u;
	tileSize.Height = 1u;
	tileSize.Width = 1u;
	tileSize.NumTiles = 1u;
	tileSize.UseBox = TRUE;
	D3D12_TILED_RESOURCE_COORDINATE newPageCoordinates[maxPagesLoading];

	unsigned char previousTextureId = newPages[0u]->allocationInfo.textureLocation.textureId;
	std::size_t lastIndex = 0u;
	std::size_t i = 0u;
	for(; i != newPageCount; ++i)
	{
		unsigned char textureId = newPages[i]->allocationInfo.textureLocation.textureId;
		if(textureId != previousTextureId)
		{
			VirtualTextureInfo& textureInfo = texturesByID[previousTextureId];
			const std::size_t pageCount = i - lastIndex;
			allocator.addPages(newPageCoordinates + lastIndex, pageCount, textureInfo.resource, previousTextureId, commandQueue, graphicsDevice, HeapLocationsIterator{newPages + lastIndex});
			addPageDataToResource(textureInfo, newPageCoordinates + lastIndex, newPages + lastIndex, pageCount, tileSize, cache,
				commandList, graphicsEngine, uploadComplete);
			lastIndex = i;
			previousTextureId = textureId;
		}

		newPageCoordinates[i].X = (UINT)newPages[i]->allocationInfo.textureLocation.x;
		newPageCoordinates[i].Y = (UINT)newPages[i]->allocationInfo.textureLocation.y;
		newPageCoordinates[i].Z = (UINT)0u;
		newPageCoordinates[i].Subresource = (UINT)newPages[i]->allocationInfo.textureLocation.mipLevel;
	}
	const std::size_t pageCount = i - lastIndex;
	VirtualTextureInfo& textureInfo = texturesByID[previousTextureId];
	allocator.addPages(newPageCoordinates + lastIndex, pageCount, textureInfo.resource, previousTextureId, commandQueue, graphicsDevice, HeapLocationsIterator{newPages + lastIndex});
	addPageDataToResource(textureInfo, newPageCoordinates + lastIndex, newPages + lastIndex, pageCount, tileSize, cache,
		commandList, graphicsEngine, uploadComplete);
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

void PageProvider::processMessages(void* tr)
{
	SinglyLinked* messages = messageQueue.popAll();
	while (messages != nullptr)
	{
		LinkedTask& message = *static_cast<LinkedTask*>(messages);
		messages = messages->next;
		message.execute(message, tr);
	}
}

template<class HashMap>
static inline void requestMipLevels(const unsigned int lowestPinnedMip, unsigned char textureId, unsigned char mipLevel, unsigned short x, unsigned short y, HashMap& uniqueRequests)
{
	if(mipLevel >= lowestPinnedMip) return;
	x >>= mipLevel;
	y >>= mipLevel;
	while(true)
	{
		unsigned long long& count = uniqueRequests[{textureId, mipLevel, x, y}].count;
		++count;
		++mipLevel;
		if(count != 1u || mipLevel == lowestPinnedMip) return;
		x >>= 1u;
		y >>= 1u;
	}
}

void PageProvider::gatherPageRequests(void* feadBackBuffer, unsigned long sizeInBytes)
{
	auto& requests = this->uniqueRequests;
	uint64_t* current = static_cast<uint64_t*>(feadBackBuffer);
	const uint64_t* feadBackBufferEnd = current + (sizeInBytes / sizeof(uint64_t));
	for(; current != feadBackBufferEnd; ++current)
	{
		const uint64_t value = *current;// pages are stored as textureId2, textureId3, textureId1, miplevel, y, x
		const unsigned char mipLevel = static_cast<unsigned char>((value & 0x000000ff00000000) >> 32u);
		const unsigned short x = static_cast<unsigned short>(value & 0x000000000000ffff);
		const unsigned short y = static_cast<unsigned short>((value & 0x00000000ffff0000) >> 16u);
		const unsigned char textureId1 = static_cast<unsigned char>((value & 0x0000ff0000000000) >> 40u);
		const unsigned char textureId2 = static_cast<unsigned char>((value & 0xff00000000000000) >> 56u);
		const unsigned char textureId3 = static_cast<unsigned char>((value & 0x00ff000000000000) >> 48u);

		if(textureId1 != 255u)
		{
			VirtualTextureInfo& textureInfo = texturesByID[textureId1];
			requestMipLevels(textureInfo.lowestPinnedMip, textureId1, mipLevel, x, y, requests);
		}
		if(textureId2 != 255u)
		{
			VirtualTextureInfo& textureInfo = texturesByID[textureId2];
			requestMipLevels(textureInfo.lowestPinnedMip, textureId2, mipLevel, x, y, requests);
		}
		if(textureId3 != 255u)
		{
			VirtualTextureInfo& textureInfo = texturesByID[textureId3];
			requestMipLevels(textureInfo.lowestPinnedMip, textureId3, mipLevel, x, y, requests);
		}
	}
}

void PageProvider::processPageRequestsHelper(IDXGIAdapter3* adapter, float& mipBias, float desiredMipBias, void* tr)
{
	processMessages(tr);
	//Work out memory budget, grow or shrink cache as required and change mip bias as required
	long long memoryBedgetInPages = calculateMemoryBudgetInPages(adapter);
	std::size_t cacheCapacity = pageCache.capacity();
	auto newCapacity = recalculateCacheSize(mipBias, desiredMipBias, memoryBedgetInPages, uniqueRequests.size(), cacheCapacity);
	while (newCapacity.mipLevelIncreased)
	{
		increaseMipBias(uniqueRequests);
		newCapacity = recalculateCacheSize(mipBias, desiredMipBias, memoryBedgetInPages, uniqueRequests.size(), newCapacity.capacity);
	}
	newPageCacheCapacity = newCapacity.capacity;

	//work out which pages are in the cache and which pages need loading
	checkCacheForPages(uniqueRequests, texturesByID, pageCache, posableLoadRequests);
	uniqueRequests.clear();
	shrinkNumberOfLoadRequestsIfNeeded(freePageLoadRequestsCount);
}

void PageProvider::increaseMipBias(decltype(uniqueRequests)& pageRequests)
{
	for(auto& pageRequest : pageRequests)
	{
		VirtualTextureInfo& textureInfo = texturesByID[pageRequest.first.textureId];
		const unsigned char newMipLevel = pageRequest.first.mipLevel + 1u;
		if(textureInfo.lowestPinnedMip == newMipLevel) continue;
		posableLoadRequests.push_back({ PageResourceLocation{pageRequest.first.textureId, newMipLevel, pageRequest.first.x, pageRequest.first.y}, pageRequest.second.count });
	}
	pageRequests.clear();
	for(auto& pageRequest : posableLoadRequests)
	{
		pageRequests[pageRequest.first].count += pageRequest.second;
	}
	posableLoadRequests.clear();
}

void PageProvider::addNewPagesToCache(ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests, VirtualTextureInfoByID& textureInfoById, PageCache& pageCache, PageDeleter& pageDeleter)
{
	for (const auto& requestInfo : posableLoadRequests)
	{
		VirtualTextureInfo& textureInfo = textureInfoById[requestInfo.first.textureId];
		pageCache.addNonAllocatedPage(requestInfo.first, textureInfo, textureInfoById, pageDeleter);
	}
	posableLoadRequests.clear();
}

void PageProvider::deleteTextureHelper(UnloadRequest& unloadRequest, void* tr)
{
	VirtualTextureInfo& textureInfo = *unloadRequest.textureInfo;
	textureInfo.file.close();
	std::size_t numberOfNewUnneededLoadingPages = 0u;
	textureInfo.pageCacheData.pageLookUp.consume([&pageCache = pageCache, &pageAllocator = pageAllocator, &numberOfNewUnneededLoadingPages](auto&& page)
	{
		pageCache.removePage(&page);
		if (page.data.heapLocation.heapOffsetInPages == std::numeric_limits<decltype(page.data.heapLocation.heapOffsetInPages)>::max())
		{
			++numberOfNewUnneededLoadingPages;
		}
		else
		{
			pageAllocator.removePage(page.data.heapLocation);
		}
	});
	pageAllocator.removePinnedPages(textureInfo.pinnedHeapLocations.get(), textureInfo.pinnedPageCount);
	textureInfo.pageCacheData.numberOfUnneededLoadingPages += numberOfNewUnneededLoadingPages;
	if (textureInfo.pageCacheData.numberOfUnneededLoadingPages == 0u)
	{
		assert(textureInfo.textureID != 255);
		texturesByID.deallocate(textureInfo.textureID);
		unloadRequest.callback(unloadRequest, tr);
	}
	else
	{
		textureInfo.unloadRequest = &unloadRequest;
	}
}

void PageProvider::allocateTexturePinnedHelper(AllocateTextureRequest& allocateRequest, void* tr)
{
	auto& textureInfo = texturesByID.allocate();
	textureInfo.resource = std::move(allocateRequest.resource);
	textureInfo.widthInPages = allocateRequest.widthInPages;
	textureInfo.heightInPages = allocateRequest.heightInPages;
	textureInfo.lowestPinnedMip = allocateRequest.lowestPinnedMip;

	ID3D12CommandQueue& commandQueue = allocateRequest.pageProvider->streamingManager.commandQueue();
	ID3D12Device& graphicsDevice = *allocateRequest.pageProvider->graphicsEngine.graphicsDevice;

	D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[64];
	constexpr size_t resourceTileCoordsMax = sizeof(resourceTileCoords) / sizeof(resourceTileCoords[0u]);
	unsigned int resourceTileCoordsIndex = 0u;

	auto widthInPages = textureInfo.widthInPages >> textureInfo.lowestPinnedMip;
	if (widthInPages == 0u) widthInPages = 1u;
	auto heightInPages = textureInfo.heightInPages >> textureInfo.lowestPinnedMip;
	if (heightInPages == 0u) heightInPages = 1u;
	textureInfo.pinnedPageCount = widthInPages * heightInPages;
	textureInfo.pinnedHeapLocations.reset(new GpuHeapLocation[textureInfo.pinnedPageCount]);
	GpuHeapLocation* pinnedHeapLocations = textureInfo.pinnedHeapLocations.get();
	for (auto y = 0u; y != heightInPages; ++y)
	{
		for (auto x = 0u; x != widthInPages; ++x)
		{
			if (resourceTileCoordsIndex == resourceTileCoordsMax)
			{
				pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsMax, textureInfo.resource, static_cast<unsigned char>(textureInfo.textureID), pinnedHeapLocations, &commandQueue, &graphicsDevice);
				resourceTileCoordsIndex = 0u;
				pinnedHeapLocations += resourceTileCoordsMax;
			}
			resourceTileCoords[resourceTileCoordsIndex].X = x;
			resourceTileCoords[resourceTileCoordsIndex].Y = y;
			resourceTileCoords[resourceTileCoordsIndex].Z = 0u;
			resourceTileCoords[resourceTileCoordsIndex].Subresource = textureInfo.lowestPinnedMip;
			++resourceTileCoordsIndex;
		}
	}
	pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsIndex, textureInfo.resource, static_cast<unsigned char>(textureInfo.textureID), pinnedHeapLocations, &commandQueue, &graphicsDevice);
	allocateRequest.callback(allocateRequest, tr, textureInfo);
}

void PageProvider::allocateTexturePackedHelper(AllocateTextureRequest& allocateRequest, void* tr)
{
	auto& textureInfo = texturesByID.allocate();
	textureInfo.resource = std::move(allocateRequest.resource);
	textureInfo.widthInPages = allocateRequest.widthInPages;
	textureInfo.heightInPages = allocateRequest.heightInPages;
	textureInfo.pinnedPageCount = allocateRequest.pinnedPageCount;
	textureInfo.lowestPinnedMip = allocateRequest.lowestPinnedMip;

	ID3D12CommandQueue& commandQueue = allocateRequest.pageProvider->streamingManager.commandQueue();
	ID3D12Device& graphicsDevice = *allocateRequest.pageProvider->graphicsEngine.graphicsDevice;

	textureInfo.pinnedHeapLocations.reset(new GpuHeapLocation[textureInfo.pinnedPageCount]);
	pageAllocator.addPackedPages(textureInfo.resource, textureInfo.pinnedHeapLocations.get(), textureInfo.lowestPinnedMip, textureInfo.pinnedPageCount, &commandQueue, &graphicsDevice);

	allocateRequest.callback(allocateRequest, tr, textureInfo);
}

void PageProvider::deleteTexture(UnloadRequest& unloadRequest)
{
	unloadRequest.pageProvider = this;
	unloadRequest.execute = [](LinkedTask& task, void* tr)
	{
		UnloadRequest& unloadRequest = static_cast<UnloadRequest&>(task);
		unloadRequest.pageProvider->deleteTextureHelper(unloadRequest, tr);
	};
	messageQueue.push(&unloadRequest);
}

void PageProvider::allocateTexturePinned(AllocateTextureRequest& allocateRequest)
{
	allocateRequest.pageProvider = this;
	allocateRequest.execute = [](LinkedTask& task, void* tr)
	{
		AllocateTextureRequest& allocateRequest = static_cast<AllocateTextureRequest&>(task);
		allocateRequest.pageProvider->allocateTexturePinnedHelper(allocateRequest, tr);
	};
	messageQueue.push(&allocateRequest);
}

void PageProvider::allocateTexturePacked(AllocateTextureRequest& allocateRequest)
{
	allocateRequest.pageProvider = this;
	allocateRequest.execute = [](LinkedTask& task, void* tr)
	{
		AllocateTextureRequest& allocateRequest = static_cast<AllocateTextureRequest&>(task);
		allocateRequest.pageProvider->allocateTexturePackedHelper(allocateRequest, tr);
	};
	messageQueue.push(&allocateRequest);
}