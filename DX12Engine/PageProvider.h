#pragma once
#include "PageAllocator.h"
#include "PageCache.h"
#include "TextureResitency.h"
#include "Range.h"
#include <atomic>
#include <vector>
#include <d3d12.h>
#include <unordered_map>
#include "D3D12Resource.h"
class VirtualTextureManager;
class StreamingManager;
struct IDXGIAdapter3;
class BaseExecutor;

class PageProvider
{
	constexpr static size_t maxPageCountMultiplierLowerBound = 4;
	constexpr static size_t maxPageCountMultiplierUpperBound = 6;
	constexpr static double memoryFullUpperBound = 0.97;
	constexpr static double memoryFullLowerBound = 0.95;
	constexpr static size_t maxPagesLoading = 32u;
public:
	PageAllocator pageAllocator;
private:
	PageCache pageCache;
	unsigned long long memoryUsage;
	signed long long maxPages; //total number of pages that can be used for non-pinned virtual texture pages. Can be negative when too much memory is being used for other things
	float mipBias;
	float desiredMipBias;

	struct PageLoadRequest
	{
		enum class State
		{
			finished,
			pending,
			unused,
		};
		std::atomic<State> state;
		PageAllocationInfo allocationInfo;
		PageProvider* pageProvider;
	};

	PageLoadRequest pageLoadRequests[maxPagesLoading];
	std::vector<std::pair<textureLocation, unsigned long long>> posableLoadRequests;
	D3D12Resource uploadResource; //this might be able to be merged into stream manager when it gets non-blocking io support
	uint8_t* uploadResourcePointer;
	PageAllocationInfo newPages[maxPagesLoading];
	unsigned int newPagesOffsetInLoadRequests[maxPagesLoading];
	unsigned int newPageCount;
	size_t newCacheSize;

	static void addPageLoadRequestHelper(PageLoadRequest& pageRequest, VirtualTextureManager& virtualTextureManager);

	template<class Executor>
	static void addPageLoadRequest(PageLoadRequest& pageRequest, BaseExecutor* executor)
	{
		executor->sharedResources->backgroundQueue.push(Job(&pageRequest, [](void* requester, BaseExecutor* exe)
		{
			Executor* executor = reinterpret_cast<Executor*>(exe);
			PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(requester);
			VirtualTextureManager& virtualTextureManager = executor->getSharedResources()->virtualTextureManager;
			addPageLoadRequestHelper(pageRequest, virtualTextureManager);
		}));
	}
public:
	PageProvider(float desiredMipBias, IDXGIAdapter3* adapter, ID3D12Device* graphicsDevice);

	template<class executor>
	void processPageRequests(std::unordered_map<textureLocation, PageRequestData>& pageRequests, VirtualTextureManager& virtualTextureManager,
		executor* executor)
	{
		IDXGIAdapter3* adapter = executor->sharedResources->graphicsEngine.adapter;
		ID3D12Device* graphicsDevice = executor->sharedResources->graphicsEngine.graphicsDevice;
		//work out page budget
		DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
		adapter->QueryVideoMemoryInfo(1u, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
		maxPages += ((signed long long)memoryUsage - (signed long long)videoMemoryInfo.CurrentUsage) / (64 * 1024);
		size_t maxPagesMinZero = maxPages > 0 ? (size_t)maxPages : 0u;
		memoryUsage = videoMemoryInfo.CurrentUsage;
		//work out how many texture pages are needed
		size_t totalPagesNeeded = pageRequests.size();

		newCacheSize = pageCache.capacity();
		unsigned int mipBiasIncrease = 0u;
		size_t pagesFullLowerBound = maxPagesMinZero * memoryFullLowerBound;
		pagesFullLowerBound -= pagesFullLowerBound % PageAllocator::heapSizeInPages;
		if (pageCache.capacity() < pagesFullLowerBound)
		{
			//we can allocate more memory
			if (mipBias > desiredMipBias)
			{
				//cache size needs increasing
				size_t newSize = maxPagesMinZero * ((memoryFullLowerBound + memoryFullUpperBound) / 2u);
				newSize -= newSize % PageAllocator::heapSizeInPages;
				pageCache.increaseSize(newSize);
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
					size_t newSize = std::min(size_t(maxPagesMinZero * ((memoryFullLowerBound + memoryFullUpperBound) / 2u)),
						totalPagesNeeded * ((maxPageCountMultiplierLowerBound + maxPageCountMultiplierUpperBound) / 2u));
					newSize -= newSize % PageAllocator::heapSizeInPages;
					pageCache.increaseSize(newSize);
				}
				else if (totalPagesNeeded * maxPageCountMultiplierUpperBound < pageCache.capacity())
				{
					//we have more memory than we need, decrease cache size
					size_t newSize = totalPagesNeeded * ((maxPageCountMultiplierLowerBound + maxPageCountMultiplierUpperBound) / 2u);
					newSize -= newSize % PageAllocator::heapSizeInPages;
					newCacheSize = newSize;
				}
			}
		}
		else
		{
			size_t pagesFullUpperBound = maxPagesMinZero * memoryFullUpperBound;
			pagesFullUpperBound -= pagesFullUpperBound % PageAllocator::heapSizeInPages;
			if (pageCache.capacity() > pagesFullUpperBound)
			{
				//memory usage must be reduced if posable
				if (totalPagesNeeded > pagesFullUpperBound)
				{
					//need to increase mipBias
					++mipBias;
					totalPagesNeeded /= 4u; //mgiht not reduce by exactly the right amount but should be close enough
				}
				//reduce cache size
				size_t newSize = size_t(maxPagesMinZero * ((memoryFullLowerBound + memoryFullUpperBound) / 2u));
				newSize -= newSize % PageAllocator::heapSizeInPages;
				newCacheSize = newSize;
			}
			else
			{
				//memory is in max range but not over it
				if (totalPagesNeeded * maxPageCountMultiplierUpperBound < pageCache.capacity())
				{
					//we have more memory than we need, decrease cache size
					size_t newSize = totalPagesNeeded * ((maxPageCountMultiplierLowerBound + maxPageCountMultiplierUpperBound) / 2u);
					newSize -= newSize % PageAllocator::heapSizeInPages;
					newCacheSize = newSize;
				}
				else if (totalPagesNeeded > pageCache.capacity())
				{
					//need to increase mipBias
					++mipBias;
					++mipBias;
					totalPagesNeeded /= 4u; //mgiht not reduce by exactly the right amount but should be close enough
				}
			}
		}

		//work out which pages are in the cache, which pages need loading and the number that can be dropped from the cache
		size_t numRequiredPagesInCache = 0u;
		for (auto& request : pageRequests)
		{
			textureLocation location;
			location.value = request.first.value;
			auto textureId = request.first.textureId();
			auto textureSlot = request.first.textureSlots();
			const unsigned int mipLevel = request.first.mipLevel() + mipBiasIncrease;
			location.setMipLevel(mipLevel);
			const VirtualTextureInfo& textureInfo = virtualTextureManager.texturesByIDAndSlot[textureSlot].data()[textureId];

			if (pageCache.getPage(location) != nullptr)
			{
				++numRequiredPagesInCache;
			}
			else
			{
				const unsigned int nextMipLevel = mipLevel + 1u;
				textureLocation nextMipLocation;
				nextMipLocation.setX(location.x() >> 1u);
				nextMipLocation.setY(location.y() >> 1u);
				nextMipLocation.setMipLevel(nextMipLevel);
				nextMipLocation.setTextureIdAndSlot(location.textureIdAndSlot());
				if (nextMipLevel == textureInfo.lowestPinnedMip || pageCache.getPage(nextMipLocation) != nullptr)
				{
					bool pageAlreadyLoading = false;
					for (const auto& pageRequest : pageLoadRequests)
					{
						auto state = pageRequest.state.load(std::memory_order::memory_order_acquire);
						if (state != PageLoadRequest::State::unused)
						{
							if (pageRequest.allocationInfo.textureLocation.value == location.value)
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

		//work out which pages have finished loading and how many textures can to requested
		newPageCount = 0u;
		unsigned int numTexturesThatCanBeRequested = 0u;
		for (auto& pageRequest : pageLoadRequests)
		{
			auto state = pageRequest.state.load(std::memory_order::memory_order_acquire);
			if (state == PageLoadRequest::State::finished)
			{
				newPages[newPageCount] = pageRequest.allocationInfo;
				newPagesOffsetInLoadRequests[newPageCount] = &pageRequest - pageLoadRequests;
				++newPageCount;
				pageRequest.state.store(PageLoadRequest::State::unused, std::memory_order::memory_order_relaxed);
				++numTexturesThatCanBeRequested;
			}
			else if (state == PageLoadRequest::State::unused)
			{
				++numTexturesThatCanBeRequested;
			}
		}

		//work out which textures can be requested
		if (numTexturesThatCanBeRequested < posableLoadRequests.size())
		{
			if (numTexturesThatCanBeRequested != 0u)
			{
				std::sort(posableLoadRequests.begin(), posableLoadRequests.end(), [](const auto& lhs, const auto& rhs)
				{
					return lhs.secound > rhs.secound; //sort in descending order of pixel count on screen
				});
			}
			//reduce posableLoadRequests size to numTexturesThatCanBeRequested
			posableLoadRequests.resize(numTexturesThatCanBeRequested);
		}
		//start all texture pages in posableLoadRequests loading
		auto loadRequestsStart = posableLoadRequests.begin();
		const auto loadRequestsEnd = posableLoadRequests.end();
		if (loadRequestsStart != loadRequestsEnd)
		{
			for (auto& pageRequest : pageLoadRequests)
			{
				auto state = pageRequest.state.load(std::memory_order::memory_order_relaxed);
				if (state == PageLoadRequest::State::unused)
				{
					pageRequest.state.store(PageLoadRequest::State::pending, std::memory_order::memory_order_relaxed);
					pageRequest.allocationInfo.textureLocation = loadRequestsStart->first;
					addPageLoadRequest(pageRequest, executor);
					++loadRequestsStart;
					if (loadRequestsStart == loadRequestsEnd) break;
				}
			}
		}
		posableLoadRequests.clear();

		//work out which newly loaded pages have to be dropped based on the number of pages that can be dropped from the cache
		size_t numDroppablePagesInCache = newCacheSize - numRequiredPagesInCache;
		if (numDroppablePagesInCache < newPageCount)
		{
			size_t numPagesToDrop = newPageCount - numDroppablePagesInCache;
			auto i = newPageCount;
			while (true)
			{
				textureLocation newPageTextureLocation = newPages[i].textureLocation;
				newPageTextureLocation.decreaseMipLevel(mipBiasIncrease);
				auto pos = pageRequests.find(newPageTextureLocation);
				if (pos == pageRequests.end())
				{
					//this page isn't needed
					--newPageCount;
				}
				else
				{
					--i;
					while (true)
					{
						textureLocation newPageTextureLocation = newPages[i].textureLocation;
						newPageTextureLocation.decreaseMipLevel(mipBiasIncrease);
						auto pos = pageRequests.find(newPageTextureLocation);
						if (pos == pageRequests.end())
						{
							//this page isn't needed
							--newPageCount;
							std::swap(newPages[i], newPages[newPageCount]);
						}
						if (i == 0u) break;
						--i;
					}
					break;
				}
				if (i == 0u) break;
				--i;
			}
		}

		executor->renderJobQueue().push(Job(this, [](void* requester, BaseExecutor* exe)
		{
			Executor* executor = reinterpret_cast<Executor*>(exe);
			ID3D12CommandQueue* commandQueue = executor->sharedResources->graphicsEngine.directCommandQueue;
			auto sharedResources = executor->getSharedResources();
			VirtualTextureManager& virtualTextureManager = sharedResources->virtualTextureManager;
			ID3D12Device* graphicsDevice = executor->sharedResources->graphicsEngine.graphicsDevice;
			PageProvider& pageProvider = *reinterpret_cast<PageProvider*>(requester);
			PageAllocator& pageAllocator = pageProvider.pageAllocator;
			PageCache& pageCache = pageProvider.pageCache;
			PageDeleter pageDeleter(pageAllocator, virtualTextureManager.texturesByIDAndSlot, commandQueue);
			size_t newCacheSize = pageProvider.newCacheSize;
			auto newPageCount = pageProvider.newPageCount;
			PageAllocationInfo* newPages = pageProvider.newPages;
			unsigned int* newPagesOffsetInLoadRequests = pageProvider.newPagesOffsetInLoadRequests;
			if (newCacheSize < pageCache.capacity())
			{
				pageCache.decreaseSize(newCacheSize, pageDeleter);
				size_t newPageAllocatorSize = newCacheSize + newCacheSize % PageAllocator::heapSizeInPages;
				pageAllocator.decreaseNonPinnedSize(newPageAllocatorSize, pageCache, commandQueue, virtualTextureManager.texturesByIDAndSlot);
			}
			//add all remaining new pages to the cache and drop old pages from the cache
			if (newPageCount != 0u)
			{
				ID3D12GraphicsCommandList* commandList = executor->renderPass.virtualTextureFeedbackSubPass.firstCommandList();
				D3D12_TILE_REGION_SIZE tileSize;
				tileSize.Depth = 1u;
				tileSize.Height = 1u;
				tileSize.Width = 1u;
				tileSize.NumTiles = 1u;
				tileSize.UseBox = TRUE;

				D3D12_TILED_RESOURCE_COORDINATE newPageCoordinates[maxPagesLoading];
				newPageCoordinates[0].X = newPages[0u].textureLocation.x();
				newPageCoordinates[0].Y = newPages[0u].textureLocation.y();
				newPageCoordinates[0].Z = 0u;
				newPageCoordinates[0].Subresource = newPages[0u].textureLocation.mipLevel();
				unsigned int previousResourceIdAndSlot = newPages[0u].textureLocation.textureIdAndSlot();
				unsigned int lastIndex = 0u;
				{
					VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByIDAndSlot[newPages[0].textureLocation.textureId()].data()[newPages[0].textureLocation.textureSlots()];
					commandList->CopyTiles(resourceInfo.resource, &newPageCoordinates[0u], &tileSize, pageProvider.uploadResource, newPagesOffsetInLoadRequests[0] * 64u * 1024u,
						D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
				}
				auto i = 1u;
				if (newPageCount != 1u)
				{
					for (; i != newPageCount; ++i)
					{
						unsigned int resourceIdAndSlot = newPages[i].textureLocation.textureIdAndSlot();
						VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByIDAndSlot[newPages[i].textureLocation.textureId()].data()[newPages[i].textureLocation.textureSlots()];
						if (resourceIdAndSlot != previousResourceIdAndSlot)
						{
							pageAllocator.addPages(newPageCoordinates + lastIndex, i - lastIndex, resourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);
							lastIndex = i;
						}
						newPageCoordinates[i].X = newPages[i].textureLocation.x();
						newPageCoordinates[i].Y = newPages[i].textureLocation.y();
						newPageCoordinates[i].Z = 0u;
						newPageCoordinates[i].Subresource = newPages[i].textureLocation.mipLevel();

						commandList->CopyTiles(resourceInfo.resource, &newPageCoordinates[i], &tileSize, pageProvider.uploadResource, newPagesOffsetInLoadRequests[i] * 64u * 1024u,
							D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
					}
				}
				VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByIDAndSlot[newPages[i].textureLocation.textureId()].data()[newPages[i].textureLocation.textureSlots()];
				pageAllocator.addPages(newPageCoordinates + lastIndex, i - lastIndex, resourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);

				size_t deletedPageCount;
				PageAllocationInfo deletedPages[maxPagesLoading];
				pageCache.addPages(newPages, newPageCount, pageDeleter);
			}
			pageDeleter.finish();
		}));
	}
};