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
#include "GpuCompletionEventManager.h"
class VirtualTextureManager;
class StreamingManager;
struct IDXGIAdapter3;
class BaseExecutor;
class HalfFinishedUploadRequest;
#undef min
#undef max

class PageProvider
{
	constexpr static size_t maxPageCountMultiplierLowerBound = 4;
	constexpr static size_t maxPageCountMultiplierUpperBound = 6;
	constexpr static double memoryFullUpperBound = 0.97;
	constexpr static double memoryFullLowerBound = 0.95;
	constexpr static size_t maxPagesLoading = 32u;

	class NewPageIterator
	{
		struct NewPage
		{
			PageAllocationInfo* page;
			unsigned long* offsetInUploadBuffer;
			PageAllocationInfo pageData;
			unsigned long offsetInUploadBufferData;
			VirtualTextureManager& virtualTextureManager;

			NewPage(PageAllocationInfo* page, unsigned long* offsetInUploadBuffer, VirtualTextureManager& virtualTextureManager) : page(page),
				offsetInUploadBuffer(offsetInUploadBuffer), virtualTextureManager(virtualTextureManager) {}

			NewPage(NewPage&& other) : virtualTextureManager(other.virtualTextureManager)
			{
				pageData = *other.page;
				offsetInUploadBufferData = *other.offsetInUploadBuffer;
				page = &pageData;
				offsetInUploadBuffer = &offsetInUploadBufferData;
			}

			void operator=(NewPage&& other)
			{
				*page = *other.page;
				*offsetInUploadBuffer = *other.offsetInUploadBuffer;
			}

			void swap(const NewPage& other)
			{
				auto pageData = *page;
				*page = *other.page;
				*other.page = pageData;
				auto offset = *offsetInUploadBuffer;
				*offsetInUploadBuffer = *other.offsetInUploadBuffer;
				*other.offsetInUploadBuffer = offset;
			}

			bool operator<(const NewPage& other);
		};
		NewPage newPage;
	public:
		using value_type = NewPage;
		using difference_type = std::ptrdiff_t;
		using pointer = NewPage*;
		using reference = NewPage&;
		using iterator_category = std::random_access_iterator_tag;


		NewPageIterator(PageAllocationInfo* page, unsigned long* offsetInUploadBuffer, VirtualTextureManager& virtualTextureManager) : 
			newPage{ page, offsetInUploadBuffer, virtualTextureManager } {}
		NewPageIterator(const NewPageIterator& e) : newPage(e.newPage.page, e.newPage.offsetInUploadBuffer, e.newPage.virtualTextureManager) {}
		NewPageIterator& operator=(const NewPageIterator& other)
		{
			newPage.page = other.newPage.page;
			newPage.offsetInUploadBuffer = other.newPage.offsetInUploadBuffer;
			return *this;
		}
		reference operator*() { return newPage; }
		NewPageIterator& operator++() 
		{
			++newPage.page;
			++newPage.offsetInUploadBuffer;
			return *this; 
		}
		NewPageIterator& operator--() 
		{
			--newPage.page;
			--newPage.offsetInUploadBuffer;
			return *this;
		}
		NewPageIterator operator++(int)
		{
			NewPageIterator tmp(*this); 
			++newPage.page;
			++newPage.offsetInUploadBuffer;
			return tmp;
		}
		NewPageIterator operator--(int)
		{
			NewPageIterator tmp(*this);
			--newPage.page;
			--newPage.offsetInUploadBuffer;
			return tmp;
		}
		difference_type operator-(const NewPageIterator& rhs) const noexcept
		{
			return newPage.page - rhs.newPage.page;
		}
		NewPageIterator operator+(difference_type n) const noexcept
		{
			NewPageIterator tmp(*this); 
			tmp.newPage.page += n;
			tmp.newPage.offsetInUploadBuffer += n;
			return tmp; 
		}
		NewPageIterator operator-(difference_type n) const noexcept
		{ 
			NewPageIterator tmp(*this);
			tmp.newPage.page -= n;
			tmp.newPage.offsetInUploadBuffer -= n;
			return tmp; 
		}
		bool operator==(const NewPageIterator& rhs) { return newPage.page == rhs.newPage.page; }
		bool operator!=(const NewPageIterator& rhs) { return newPage.page != rhs.newPage.page; }
		bool operator<(const NewPageIterator& rhs) { return newPage.page < rhs.newPage.page; }
	};
public:
	PageAllocator pageAllocator;
	float mipBias;
private:
	PageCache pageCache;
	unsigned long long memoryUsage;
	signed long long maxPages; //total number of pages that can be used for non-pinned virtual texture pages. Can be negative when too much memory is being used for other things
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
		const wchar_t* filename;
		uint64_t offsetInFile;
		HalfFinishedUploadRequest* subresourceRequest;
	};

	PageLoadRequest pageLoadRequests[maxPagesLoading];
	std::vector<std::pair<textureLocation, unsigned long long>> posableLoadRequests;
	PageAllocationInfo newPages[maxPagesLoading];
	unsigned long newPagesOffsetInLoadRequests[maxPagesLoading];
	unsigned int newPageCount;
	size_t newCacheSize;

	static void addPageLoadRequestHelper(PageLoadRequest& pageRequest, VirtualTextureManager& virtualTextureManager, SharedResources& sharedResources);

	template<class SharedResources_t>
	static void addPageLoadRequest(PageLoadRequest& pageRequest, SharedResources_t& sharedResources)
	{
		sharedResources.backgroundQueue.push(Job(&pageRequest, [](void* requester, BaseExecutor* exe, SharedResources& sr)
		{
			SharedResources_t& sharedResources = reinterpret_cast<SharedResources_t&>(sr);
			PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(requester);
			VirtualTextureManager& virtualTextureManager = sharedResources.virtualTextureManager;
			addPageLoadRequestHelper(pageRequest, virtualTextureManager, sharedResources);
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
		adapter->QueryVideoMemoryInfo(0u, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
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

		//work out which pages have finished loading and how many textures can to requested
		newPageCount = 0u;
		unsigned int numTexturesThatCanBeRequested = 0u;
		for (auto& pageRequest : pageLoadRequests)
		{
			auto state = pageRequest.state.load(std::memory_order::memory_order_acquire);
			if (state == PageLoadRequest::State::finished)
			{
				newPages[newPageCount] = pageRequest.allocationInfo;
				newPagesOffsetInLoadRequests[newPageCount] = static_cast<unsigned long>(&pageRequest - pageLoadRequests);
				++newPageCount;
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
				auto state = pageRequest.state.load(std::memory_order::memory_order_acquire);
				if (state == PageLoadRequest::State::unused)
				{
					pageRequest.state.store(PageLoadRequest::State::pending, std::memory_order::memory_order_relaxed);
					pageRequest.allocationInfo.textureLocation = loadRequestsStart->first;
					addPageLoadRequest(pageRequest, sharedResources);
					++loadRequestsStart;
					if (loadRequestsStart == loadRequestsEnd) break;
				}
			}
			posableLoadRequests.clear();
		}

		//work out which newly loaded pages have to be dropped based on the number of pages that can be dropped from the cache
		size_t numDroppablePagesInCache = newCacheSize - numRequiredPagesInCache;
		if (numDroppablePagesInCache < newPageCount)
		{
			size_t numPagesToDrop = newPageCount - numDroppablePagesInCache;
			if (mipBiasIncrease != 0u)
			{
				//very hard to tell if a page needs loading when mip level has decreased. Could check if all four lower mips are requested
				newPageCount -= (unsigned int)numPagesToDrop;
			}
			else
			{
				auto i = newPageCount;
				while (true)
				{
					--i;
					auto pos = pageRequests.find(newPages[i].textureLocation);
					if (pos == pageRequests.end())
					{
						//this page isn't needed
						--newPageCount;
					}
					else
					{
						while (i != 0u)
						{
							--i;
							auto pos = pageRequests.find(newPages[i].textureLocation);
							if (pos == pageRequests.end())
							{
								//this page isn't needed
								--newPageCount;
								std::swap(newPages[i], newPages[newPageCount]);
								std::swap(newPagesOffsetInLoadRequests[i], newPagesOffsetInLoadRequests[newPageCount]);
							}
						}
						break;
					}
					if (i == 0u) break;
				}
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
			GpuCompletionEventManager& gpuCompletionEventManager = executor->gpuCompletionEventManager;
			PageAllocator& pageAllocator = pageProvider.pageAllocator;
			PageCache& pageCache = pageProvider.pageCache;
			PageDeleter<decltype(virtualTextureManager.texturesByID)> pageDeleter(pageAllocator, virtualTextureManager.texturesByID, commandQueue);
			size_t newCacheSize = pageProvider.newCacheSize;
			auto newPageCount = pageProvider.newPageCount;
			PageAllocationInfo* newPages = pageProvider.newPages;
			unsigned long* newPagesOffsetInLoadRequests = pageProvider.newPagesOffsetInLoadRequests;
			if (newCacheSize < pageCache.capacity())
			{
				pageAllocator.decreaseNonPinnedSize(newCacheSize, pageCache, commandQueue, virtualTextureManager.texturesByID);
				pageCache.decreaseSize(newCacheSize, pageDeleter);
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

				//sort in order of resource
				std::sort(NewPageIterator{ newPages, newPagesOffsetInLoadRequests, virtualTextureManager },
					NewPageIterator{ newPages + newPageCount, newPagesOffsetInLoadRequests + newPageCount, virtualTextureManager });

				VirtualTextureInfo* previousResourceInfo = &virtualTextureManager.texturesByID[newPages[0u].textureLocation.textureId1()];
				unsigned int lastIndex = 0u;
				auto i = 0u;
				for (; i != newPageCount; ++i)
				{
					newPageCoordinates[i].X = (UINT)newPages[i].textureLocation.x();
					newPageCoordinates[i].Y = (UINT)newPages[i].textureLocation.y();
					newPageCoordinates[i].Z = 0u;
					newPageCoordinates[i].Subresource = (UINT)newPages[i].textureLocation.mipLevel();

					VirtualTextureInfo* resourceInfo = &virtualTextureManager.texturesByID[newPages[i].textureLocation.textureId1()];
					if (resourceInfo != previousResourceInfo)
					{
						const unsigned int pageCount = i - lastIndex;
						pageAllocator.addPages(newPageCoordinates + lastIndex, pageCount, *previousResourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);
						pageProvider.addPageDataToResource(previousResourceInfo->resource, &newPageCoordinates[lastIndex], pageCount, tileSize, &newPagesOffsetInLoadRequests[lastIndex],
							commandList, gpuCompletionEventManager, sharedResources.graphicsEngine.frameIndex);
						lastIndex = i;
						previousResourceInfo = resourceInfo;
					}
				}
				const unsigned int pageCount = i - lastIndex;
				pageAllocator.addPages(newPageCoordinates + lastIndex, pageCount, *previousResourceInfo, commandQueue, graphicsDevice, newPages + lastIndex);
				pageProvider.addPageDataToResource(previousResourceInfo->resource, &newPageCoordinates[lastIndex], pageCount, tileSize, &newPagesOffsetInLoadRequests[lastIndex],
					commandList, gpuCompletionEventManager, sharedResources.graphicsEngine.frameIndex);
				pageCache.addPages(newPages, newPageCount, pageDeleter);
			}
			pageDeleter.finish();
		}));
	}

	void addPageDataToResource(ID3D12Resource* resource, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, unsigned int pageCount,
		D3D12_TILE_REGION_SIZE& tileSize, unsigned long* uploadBufferOffsets, ID3D12GraphicsCommandList* commandList, GpuCompletionEventManager& gpuCompletionEventManager, uint32_t frameIndex);
};