#pragma once
#include "PageAllocator.h"
#include "PageCache.h"
#include "PageAllocationInfo.h"
#include "Range.h"
#include <atomic>
#include "ResizingArray.h"
#include <d3d12.h>
#include "D3D12Resource.h"
#include "PageDeleter.h" 
#include "GpuCompletionEventManager.h"
#include "frameBufferCount.h"
#include "AsynchronousFileManager.h"
#include "StreamingManager.h"
class VirtualTextureManager;
struct IDXGIAdapter3;
class D3D12GraphicsEngine;
class FeedbackAnalizerSubPass;
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
				auto pageTemp = *page;
				*page = *other.page;
				*other.page = pageTemp;
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

	class PageLoadRequest : public StreamingManager::StreamingRequest, public AsynchronousFileManager::IORequest
	{
	public:
		unsigned long pageWidthInBytes;
		unsigned long widthInBytes;
		unsigned long heightInTexels;

		enum class State
		{
			finished,
			pending,
			unused,
			waitingToFreeMemory,
		};
		std::atomic<State> state;
		PageAllocationInfo allocationInfo;
		//uint64_t offsetInFile;
	};
public:
	PageAllocator pageAllocator;
private:
	PageCache pageCache;
	unsigned long long memoryUsage;
	signed long long maxPages; //total number of pages that can be used for non-pinned virtual texture pages. Can be negative when too much memory is being used for other things

	PageLoadRequest pageLoadRequests[maxPagesLoading];
	ResizingArray<std::pair<TextureLocation, unsigned long long>> posableLoadRequests;
	PageAllocationInfo newPages[maxPagesLoading];
	unsigned long newPageOffsetsInLoadRequests[maxPagesLoading];
	size_t newPageCount;
	size_t newCacheSize;

	static void copyPageToUploadBuffer(StreamingManager::StreamingRequest* useSubresourceRequest, const unsigned char* data);
	static void addPageLoadRequestHelper(PageLoadRequest& pageRequest, VirtualTextureManager& virtualTextureManager, 
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources));

	template<class ThreadResources, class GlobalResources>
	static void addPageLoadRequest(PageLoadRequest& pageRequest, ThreadResources& threadResources, GlobalResources&)
	{
		threadResources.taskShedular.backgroundQueue().push({ &pageRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& sharedResources)
		{
			PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(requester);
			VirtualTextureManager& virtualTextureManager = sharedResources.virtualTextureManager;
			addPageLoadRequestHelper(pageRequest, virtualTextureManager, [](StreamingManager::StreamingRequest* request, void* tr, void* gr)
			{
				PageLoadRequest& uploadRequest = *static_cast<PageLoadRequest*>(request);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);

				uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void*, void*, const unsigned char* buffer)
				{
					PageLoadRequest& uploadRequest = static_cast<PageLoadRequest&>(request);
					copyPageToUploadBuffer(&uploadRequest, buffer);
					uploadRequest.state.store(PageProvider::PageLoadRequest::State::finished, std::memory_order::memory_order_release);
				};
				globalResources.asynchronousFileManager.readFile(tr, gr, &uploadRequest);
			});
			StreamingManager& streamingManager = sharedResources.streamingManager;
			streamingManager.addUploadRequest(&pageRequest, threadResources, sharedResources);
		} });
	}

	unsigned int recalculateCacheSize(FeedbackAnalizerSubPass& feedbackAnalizerSubPass, IDXGIAdapter3* adapter, size_t totalPagesNeeded);
	void workoutIfPageNeededInCache(unsigned int mipBiasIncrease, std::pair<const TextureLocation, PageRequestData>& request, VirtualTextureManager& virtualTextureManager, size_t& numRequiredPagesInCache);
	size_t findLoadedTexturePages();

	template<class ThreadResources, class GlobalResources>
	void startLoadingRequiredPages(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		auto loadRequestsCurrent = posableLoadRequests.begin();
		const auto loadRequestsEnd = posableLoadRequests.end();
		if (loadRequestsCurrent != loadRequestsEnd)
		{
			for (auto& pageRequest : pageLoadRequests)
			{
				auto state = pageRequest.state.load(std::memory_order::memory_order_acquire);
				if (state == PageLoadRequest::State::unused)
				{
					pageRequest.state.store(PageLoadRequest::State::pending, std::memory_order::memory_order_relaxed);
					pageRequest.allocationInfo.textureLocation = loadRequestsCurrent->first;
					addPageLoadRequest(pageRequest, threadResources, globalResources);
					++loadRequestsCurrent;
					if (loadRequestsCurrent == loadRequestsEnd) break;
				}
			}
			posableLoadRequests.clear();
		}
	}

	template<class HashMap>
	void dropNewPagesIfTooMany(HashMap& pageRequests, size_t numRequiredPagesInCache, unsigned int mipBiasIncrease)
	{
		size_t numDroppablePagesInCache = newCacheSize - numRequiredPagesInCache;
		if (numDroppablePagesInCache < newPageCount)
		{
			if (mipBiasIncrease != 0u)
			{
				//very hard to tell if a page needs loading when mip level has changed. Therefor we just keep the first pages.
				newPageCount = numDroppablePagesInCache;
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
						//this page isn't needed because it wasn't requested this frame
						--newPageCount;
					}
					else
					{
						while (i != 0u)
						{
							--i;
							pos = pageRequests.find(newPages[i].textureLocation);
							if (pos == pageRequests.end())
							{
								//this page isn't needed because it wasn't requested this frame
								--newPageCount;
								std::swap(newPages[i], newPages[newPageCount]);
								std::swap(newPageOffsetsInLoadRequests[i], newPageOffsetsInLoadRequests[newPageCount]);
							}
						}
						break;
					}
					if (i == 0u) break;
				}
			}
		}
	}

	static void addNewPagesToResources(PageProvider& pageProvider, D3D12GraphicsEngine& graphicsEngine, VirtualTextureManager& virtualTextureManager,
		GpuCompletionEventManager<frameBufferCount>& gpuCompletionEventManager, ID3D12GraphicsCommandList* commandList,
		void(*uploadComplete)(void*, void*, void*));

	void addPageDataToResource(ID3D12Resource* resource, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, size_t pageCount,
		D3D12_TILE_REGION_SIZE& tileSize, unsigned long* uploadBufferOffsets, ID3D12GraphicsCommandList* commandList, GpuCompletionEventManager<frameBufferCount>& gpuCompletionEventManager, uint32_t frameIndex,
		void(*uploadComplete)(void*, void*, void*));
public:
	PageProvider(IDXGIAdapter3* adapter);

	template<class ThreadResources, class GlobalResources, class HashMap>
	void processPageRequests(HashMap& pageRequests, VirtualTextureManager& virtualTextureManager,
		ThreadResources& executor, GlobalResources& sharedResources)
	{
		IDXGIAdapter3* adapter = sharedResources.graphicsEngine.adapter;
		FeedbackAnalizerSubPass& feedbackAnalizerSubPass = sharedResources.renderPass.virtualTextureFeedbackSubPass();

		//Work out memory budget, grow or shrink cache as required and change mip bias if required
		const unsigned int mipBiasIncrease = recalculateCacheSize(feedbackAnalizerSubPass, adapter, pageRequests.size());

		//work out which pages are in the cache, which pages need loading and the number that are required in the cache
		size_t numRequiredPagesInCache = 0u;
		for (auto& request : pageRequests)
		{
			workoutIfPageNeededInCache(mipBiasIncrease, request, virtualTextureManager, numRequiredPagesInCache);
		}

		//work out which pages have finished loading and how many textures can to requested
		size_t numTexturesThatCanBeRequested = findLoadedTexturePages();

		//work out which textures can be requested
		if (numTexturesThatCanBeRequested < posableLoadRequests.size())
		{
			if (numTexturesThatCanBeRequested != 0u)
			{
				std::nth_element(posableLoadRequests.begin(), posableLoadRequests.begin() + numTexturesThatCanBeRequested, posableLoadRequests.end(), [](const auto& lhs, const auto& rhs)
				{
					return lhs.second > rhs.second; //sort in descending order of area on screen
				});
			}
			//reduce posableLoadRequests size to numTexturesThatCanBeRequested
			posableLoadRequests.resize(numTexturesThatCanBeRequested);
		}
		//start all texture pages in posableLoadRequests loading
		startLoadingRequiredPages(executor, sharedResources);

		//work out which newly loaded pages have to be dropped based on the number of pages that can be dropped from the cache
		dropNewPagesIfTooMany(pageRequests, numRequiredPagesInCache, mipBiasIncrease);

		executor.taskShedular.update2NextQueue().concurrentPush({ this, [](void* requester, ThreadResources& threadResources, GlobalResources& sharedResources)
		{
			addNewPagesToResources(*reinterpret_cast<PageProvider*>(requester), sharedResources.graphicsEngine, sharedResources.virtualTextureManager, threadResources.gpuCompletionEventManager,
				threadResources.renderPass.colorSubPass().opaqueCommandList(), [](void* requester, void* tr, void* gr) //must use colorSubPass as virtualTextureFeedbackSubPass isn't in view and will have closed command lists
			{
				PageLoadRequest* request = static_cast<PageLoadRequest*>(requester);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				StreamingManager& streamingManager = globalResources.streamingManager;
				streamingManager.uploadFinished(request, threadResources, globalResources);
			});

			// we have finished with the readback resources so we can render again
			FeedbackAnalizerSubPass& feedbackAnalizerSubPass = sharedResources.renderPass.virtualTextureFeedbackSubPass();
			threadResources.taskShedular.update1NextQueue().push({ &feedbackAnalizerSubPass, [](void* requester, ThreadResources&, GlobalResources&)
			{
				FeedbackAnalizerSubPass& subPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);
				subPass.setInView();
			} });
		} });
	}
};