#pragma once
#include "PageAllocator.h"
#include "PageCache.h"
#include "TextureResitency.h"
#include "Range.h"
#include <atomic>
#include <vector>
#include <d3d12.h>
#include "D3D12Resource.h"
#include "PageDeleter.h" 
class VirtualTextureManager;
class StreamingManager;
struct IDXGIAdapter3;
class BaseExecutor;
#undef min
#undef max

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

	template<class SharedResources_t>
	static void addPageLoadRequest(PageLoadRequest& pageRequest, SharedResources_t& sharedResources)
	{
		sharedResources.backgroundQueue.push(Job(&pageRequest, [](void* requester, BaseExecutor* exe, SharedResources& sr)
		{
			SharedResources_t& sharedResources = reinterpret_cast<SharedResources_t&>(sr);
			PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(requester);
			VirtualTextureManager& virtualTextureManager = sharedResources.virtualTextureManager;
			addPageLoadRequestHelper(pageRequest, virtualTextureManager);
		}));
	}
public:
	PageProvider(float desiredMipBias, IDXGIAdapter3* adapter, ID3D12Device* graphicsDevice);

	template<class executor, class SharedResources_t, class HashMap>
	void processPageRequests(HashMap& pageRequests, VirtualTextureManager& virtualTextureManager,
		executor* executor, SharedResources_t& sharedResources)
	{
		IDXGIAdapter3* adapter = sharedResources.graphicsEngine.adapter;
		ID3D12Device* graphicsDevice = sharedResources.graphicsEngine.graphicsDevice;
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
		size_t pagesFullLowerBound = static_cast<size_t>(maxPagesMinZero * memoryFullLowerBound);
		pagesFullLowerBound -= pagesFullLowerBound % PageAllocator::heapSizeInPages;
		if (pageCache.capacity() < pagesFullLowerBound)
		{
			//we can allocate more memory
			if (mipBias > desiredMipBias)
			{
				//cache size needs increasing
				size_t newSize = static_cast<size_t>(maxPagesMinZero * ((memoryFullLowerBound + memoryFullUpperBound) / 2u));
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
			size_t pagesFullUpperBound = static_cast<size_t>(maxPagesMinZero * memoryFullUpperBound);
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
			auto textureId = request.first.textureId1();
			const unsigned int mipLevel = static_cast<unsigned int>(request.first.mipLevel()) + mipBiasIncrease;
			location.setMipLevel(mipLevel);
			const VirtualTextureInfo& textureInfo = virtualTextureManager.texturesByID.data()[textureId];

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
				nextMipLocation.setTextureId1(textureId);
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
				newPagesOffsetInLoadRequests[newPageCount] = static_cast<unsigned int>(&pageRequest - pageLoadRequests);
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
					return lhs.second > rhs.second; //sort in descending order of pixel count on screen
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
					addPageLoadRequest(pageRequest, sharedResources);
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

		executor->renderJobQueue().push(Job(this, [](void* requester, BaseExecutor* exe, SharedResources& sr)
		{
			Executor* executor = reinterpret_cast<Executor*>(exe);
			auto& sharedResources = reinterpret_cast<SharedResources_t&>(sr);
			ID3D12CommandQueue* commandQueue = sharedResources.graphicsEngine.directCommandQueue;
			VirtualTextureManager& virtualTextureManager = sharedResources.virtualTextureManager;
			ID3D12Device* graphicsDevice = sharedResources.graphicsEngine.graphicsDevice;
			PageProvider& pageProvider = *reinterpret_cast<PageProvider*>(requester);
			PageAllocator& pageAllocator = pageProvider.pageAllocator;
			PageCache& pageCache = pageProvider.pageCache;
			PageDeleter<decltype(virtualTextureManager.texturesByID)> pageDeleter(pageAllocator, virtualTextureManager.texturesByID, commandQueue);
			size_t newCacheSize = pageProvider.newCacheSize;
			auto newPageCount = pageProvider.newPageCount;
			PageAllocationInfo* newPages = pageProvider.newPages;
			unsigned int* newPagesOffsetInLoadRequests = pageProvider.newPagesOffsetInLoadRequests;
			if (newCacheSize < pageCache.capacity())
			{
				pageCache.decreaseSize(newCacheSize, pageDeleter);
				size_t newPageAllocatorSize = newCacheSize + newCacheSize % PageAllocator::heapSizeInPages;
				pageAllocator.decreaseNonPinnedSize(newPageAllocatorSize, pageCache, commandQueue, virtualTextureManager.texturesByID);
			}
			//add all remaining new pages to the cache and drop old pages from the cache
			if (newPageCount != 0u)
			{
				ID3D12GraphicsCommandList* commandList = executor->renderPass.virtualTextureFeedbackSubPass().firstCommandList();
				D3D12_TILE_REGION_SIZE tileSize;
				tileSize.Depth = 1u;
				tileSize.Height = 1u;
				tileSize.Width = 1u;
				tileSize.NumTiles = 1u;
				tileSize.UseBox = TRUE;

				D3D12_TILED_RESOURCE_COORDINATE newPageCoordinates[maxPagesLoading];
				newPageCoordinates[0].X = (UINT)newPages[0u].textureLocation.x();
				newPageCoordinates[0].Y = (UINT)newPages[0u].textureLocation.y();
				newPageCoordinates[0].Z = 0u;
				newPageCoordinates[0].Subresource = (UINT)newPages[0u].textureLocation.mipLevel();
				unsigned int previousResourceId = (unsigned int)newPages[0u].textureLocation.textureId1();
				unsigned int lastIndex = 0u;
				{
					VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID.data()[newPages[0].textureLocation.textureId1()];
					commandList->CopyTiles(resourceInfo.resource, &newPageCoordinates[0u], &tileSize, pageProvider.uploadResource, newPagesOffsetInLoadRequests[0] * 64u * 1024u,
						D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
				}
				auto i = 1u;
				if (newPageCount != 1u)
				{
					for (; i != newPageCount; ++i)
					{
						unsigned int resourceId = (unsigned int)newPages[i].textureLocation.textureId1();
						VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID.data()[newPages[i].textureLocation.textureId1()];
						if (resourceId != previousResourceId)
						{
							pageAllocator.addPages(newPageCoordinates + lastIndex, i - lastIndex, resourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);
							lastIndex = i;
							previousResourceId = resourceId;
						}
						newPageCoordinates[i].X = (UINT)newPages[i].textureLocation.x();
						newPageCoordinates[i].Y = (UINT)newPages[i].textureLocation.y();
						newPageCoordinates[i].Z = 0u;
						newPageCoordinates[i].Subresource = (UINT)newPages[i].textureLocation.mipLevel();

						commandList->CopyTiles(resourceInfo.resource, &newPageCoordinates[i], &tileSize, pageProvider.uploadResource, newPagesOffsetInLoadRequests[i] * 64u * 1024u,
							D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
					}
				}
				VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID.data()[newPages[i].textureLocation.textureId1()];
				pageAllocator.addPages(newPageCoordinates + lastIndex, i - lastIndex, resourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);

				pageCache.addPages(newPages, newPageCount, pageDeleter);
			}
			pageDeleter.finish();
		}));
	}
};