#pragma once
#include "PageAllocator.h"
#include "PageCache.h"
#include "GpuHeapLocation.h"
#include "PageAllocationInfo.h"
#include "Range.h"
#include "UnorderedMultiProducerSingleConsumerQueue.h"
#include "ResizingArray.h"
#include <d3d12.h>
#include "D3D12Resource.h"
#include "PageDeleter.h" 
#include "GraphicsEngine.h"
#include "frameBufferCount.h"
#include "AsynchronousFileManager.h"
#include "StreamingManager.h"
#include "LinkedTask.h"
#include "PrimaryTaskFromOtherThreadQueue.h"
#include "VirtualTextureInfoByID.h"
#include "TaskShedular.h"
class VirtualTextureManager;
struct IDXGIAdapter3;
class VirtualFeedbackSubPass;
#undef min
#undef max

class PageProvider : private PrimaryTaskFromOtherThreadQueue::Task
{
	constexpr static std::size_t maxPageCountMultiplierLowerBound = 4;
	constexpr static double memoryFullUpperBound = 0.97;
	constexpr static double memoryFullLowerBound = 0.95;
	constexpr static std::size_t maxPagesLoading = 32u;

	class PageLoadRequest : public StreamingManager::StreamingRequest, public AsynchronousFileManager::ReadRequest, public LinkedTask
	{
	public:
		unsigned long pageWidthInBytes;
		unsigned long widthInBytes;
		unsigned long heightInTexels;
		PageAllocationInfo allocationInfo;
		PageProvider* pageProvider;
	};

	class HeapLocationsIterator
	{
		PageLoadRequest** pageLoadRequests;
	public:
		HeapLocationsIterator(PageLoadRequest** pageLoadRequests) : pageLoadRequests(pageLoadRequests) {}

		GpuHeapLocation& operator[](std::size_t i)
		{
			return pageLoadRequests[i]->allocationInfo.heapLocation;
		}
	};

	struct PageRequestData
	{
		unsigned long long count = 0u;
	};
public:
	using UnloadRequest = VirtualTextureInfo::UnloadRequest;

	class AllocateTextureRequest : private LinkedTask
	{
		friend class PageProvider;
		PageProvider* pageProvider;
	public:
		void(*callback)(AllocateTextureRequest& unloadRequest, void* tr, VirtualTextureInfo& textureInfo);
		D3D12Resource resource;
		unsigned long pinnedPageCount;
		unsigned int widthInPages;
		unsigned int heightInPages;
		unsigned int lowestPinnedMip;

		AllocateTextureRequest() {}
	};
private:
	PageCache pageCache;
	PageAllocator pageAllocator;
	std::size_t newPageCacheCapacity;
	VirtualTextureInfoByID texturesByID;
	std::unordered_map<PageResourceLocation, PageRequestData, PageResourceLocation::Hash> uniqueRequests;
	ResizingArray<std::pair<PageResourceLocation, unsigned long long>> posableLoadRequests;
	PageLoadRequest pageLoadRequests[maxPagesLoading];
	std::size_t freePageLoadRequestsCount;
	PageLoadRequest* freePageLoadRequests;
	UnorderedMultiProducerSingleConsumerQueue messageQueue;
	UnorderedMultiProducerSingleConsumerQueue halfFinishedPageLoadRequests; //page is in cpu memory waiting to be copied to gpu memory
	VirtualFeedbackSubPass& feedbackAnalizerSubPass;
	StreamingManager& streamingManager;
	GraphicsEngine& graphicsEngine;
	AsynchronousFileManager& asynchronousFileManager;

	static void copyPageToUploadBuffer(StreamingManager::StreamingRequest* useSubresourceRequest, const unsigned char* data) noexcept;
	static void addPageLoadRequestHelper(PageLoadRequest& pageRequest,
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* tr),
		void(*resourceUploaded)(StreamingManager::StreamingRequest* request, void* tr));

	template<class ThreadResources>
	static void addPageLoadRequest(PageLoadRequest& pageRequest, ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushBackgroundTask({ &pageRequest, [](void* requester, ThreadResources& threadResources)
		{
			PageLoadRequest& pageRequest = *static_cast<PageLoadRequest*>(requester);
			addPageLoadRequestHelper(pageRequest, [](StreamingManager::StreamingRequest* request, void*)
			{
				PageLoadRequest& uploadRequest = *static_cast<PageLoadRequest*>(request);

				uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& req, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* buffer)
				{
					PageLoadRequest& request = static_cast<PageLoadRequest&>(req);

					copyPageToUploadBuffer(&request, buffer);
					request.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& req, void*)
					{
						PageLoadRequest& request = static_cast<PageLoadRequest&>(req);
						PageProvider& pageProvider = *request.pageProvider;

						pageProvider.halfFinishedPageLoadRequests.push(&static_cast<LinkedTask&>(request));
					};
					asynchronousFileManager.discard(request);
				};
				uploadRequest.pageProvider->asynchronousFileManager.readFile(uploadRequest);
			}, [](StreamingManager::StreamingRequest* requester, void*)
			{
				PageLoadRequest& request = static_cast<PageLoadRequest&>(*requester);
				PageProvider& pageProvider = *request.pageProvider;

				request.execute = [](LinkedTask& task, void*)
				{
					PageLoadRequest& pageRequest = static_cast<PageLoadRequest&>(task);
					PageProvider& pageProvider = *pageRequest.pageProvider;
					++pageProvider.freePageLoadRequestsCount;
					static_cast<LinkedTask&>(pageRequest).next = static_cast<LinkedTask*>(pageProvider.freePageLoadRequests);
					pageProvider.freePageLoadRequests = &pageRequest;
				};
				pageProvider.messageQueue.push(static_cast<LinkedTask*>(&request));
			});
			StreamingManager& streamingManager = pageRequest.pageProvider->streamingManager;
			streamingManager.addUploadRequest(&pageRequest, threadResources);
		} });
	}

	template<class ThreadResources>
	void startLoadingRequiredPages(ThreadResources& threadResources)
	{
		for (const auto& requestInfo : posableLoadRequests)
		{
			PageLoadRequest& pageRequest = *freePageLoadRequests;
			freePageLoadRequests = static_cast<PageLoadRequest*>(static_cast<LinkedTask*>(static_cast<LinkedTask*>(freePageLoadRequests)->next));
			pageRequest.allocationInfo.textureLocation = requestInfo.first;
			addPageLoadRequest(pageRequest, threadResources);
		}
		freePageLoadRequestsCount -= posableLoadRequests.size();
	}
	static void addNewPagesToCache(ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests, VirtualTextureInfoByID& textureInfoById, PageCache& pageCache, PageDeleter& pageDeleter);
	void addNewPagesToResources(ID3D12GraphicsCommandList* commandList, void(*uploadComplete)(LinkedTask& task, void* tr), void* tr);
	static void addPageDataToResource(VirtualTextureInfo& textureInfo, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, PageLoadRequest** pageLoadRequests, std::size_t pageCount,
		D3D12_TILE_REGION_SIZE& tileSize, PageCache& pageCache, ID3D12GraphicsCommandList* commandList, GraphicsEngine& graphicsEngine,
		void(*uploadComplete)(PrimaryTaskFromOtherThreadQueue::Task& task, void* tr));
	static void checkCacheForPage(std::pair<const PageResourceLocation, PageRequestData>& request, VirtualTextureInfoByID& texturesByID, PageCache& pageCache,
		ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests);
	static void checkCacheForPages(decltype(uniqueRequests)& pageRequests, VirtualTextureInfoByID& texturesByID, PageCache& pageCache,
		ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests);
	void shrinkNumberOfLoadRequestsIfNeeded(std::size_t numTexturePagesThatCanBeRequested);
	void processMessages(void* tr);

	struct NewCacheCapacity
	{
		bool mipLevelIncreased;
		std::size_t capacity;
	};
	static NewCacheCapacity recalculateCacheSize(float& mipBias, float desiredMipBias, long long maxPages, std::size_t totalPagesNeeded, std::size_t oldCacheCapacity);

	long long calculateMemoryBudgetInPages(IDXGIAdapter3* adapter);
	void processPageRequestsHelper(IDXGIAdapter3* adapter, float& mipBias, float desiredMipBias, void* tr);
	void increaseMipBias(decltype(uniqueRequests)& pageRequests);

	void deleteTextureHelper(UnloadRequest& unloadRequest, void* tr);
	void allocateTexturePinnedHelper(AllocateTextureRequest& allocateRequest, void* tr);
	void allocateTexturePackedHelper(AllocateTextureRequest& allocateRequest, void* tr);
public:
	PageProvider(VirtualFeedbackSubPass& feedbackAnalizerSubPass, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager);

	void gatherPageRequests(void* feadBackBuffer, unsigned long sizeInBytes);

	template<class ThreadResources>
	void processPageRequests(ThreadResources& threadResources, TaskShedular<ThreadResources>& taskShedular, float& mipBias, float desiredMipBias)
	{
		processPageRequestsHelper(graphicsEngine.adapter, mipBias, desiredMipBias, &threadResources);
		startLoadingRequiredPages(threadResources);

		execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr)
		{
			PageProvider& pageProvider = static_cast<PageProvider&>(task);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);

			//we have finished with the readback resources so we can render again
			VirtualFeedbackSubPass& feedbackAnalizerSubPass = pageProvider.feedbackAnalizerSubPass;
			feedbackAnalizerSubPass.setInView();

			threadResources.taskShedular.pushPrimaryTask(1u, {&pageProvider, [](void* requester, ThreadResources& threadResources)
			{
				ID3D12GraphicsCommandList* commandList = threadResources.renderPass.virtualTextureFeedbackSubPass().firstCommandList();
				PageProvider& pageProvider = *static_cast<PageProvider*>(requester);

				if (pageProvider.newPageCacheCapacity < pageProvider.pageCache.capacity())
				{
					PageDeleter pageDeleter(pageProvider.pageAllocator, pageProvider.graphicsEngine.directCommandQueue);
					if (!pageProvider.posableLoadRequests.empty())
					{
						addNewPagesToCache(pageProvider.posableLoadRequests, pageProvider.texturesByID, pageProvider.pageCache, pageDeleter);
					}
					pageProvider.pageCache.decreaseCapacity(pageProvider.newPageCacheCapacity, pageProvider.texturesByID, pageDeleter);
					pageDeleter.finish(pageProvider.texturesByID);
					pageProvider.pageAllocator.decreaseNonPinnedCapacity(pageProvider.newPageCacheCapacity, pageProvider.texturesByID, pageProvider.graphicsEngine, *commandList);
				}
				else
				{
					if (pageProvider.newPageCacheCapacity > pageProvider.pageCache.capacity())
					{
						pageProvider.pageCache.increaseCapacity(pageProvider.newPageCacheCapacity);
					}
					if (!pageProvider.posableLoadRequests.empty())
					{
						PageDeleter pageDeleter(pageProvider.pageAllocator, pageProvider.graphicsEngine.directCommandQueue);
						addNewPagesToCache(pageProvider.posableLoadRequests, pageProvider.texturesByID, pageProvider.pageCache, pageDeleter);
						pageDeleter.finish(pageProvider.texturesByID);
					}
				}
				pageProvider.addNewPagesToResources(commandList, [](LinkedTask& task, void* tr)
				{
					PageLoadRequest& request = static_cast<PageLoadRequest&>(task);
					ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
					StreamingManager& streamingManager = request.pageProvider->streamingManager;
					streamingManager.uploadFinished(&request, threadResources);
				}, &threadResources);
			}});
		};
		taskShedular.pushPrimaryTaskFromOtherThread(0u, *this);
	}

	void deleteTexture(UnloadRequest& unloadRequest);
	void allocateTexturePinned(AllocateTextureRequest& allocateRequest);
	void allocateTexturePacked(AllocateTextureRequest& allocateRequest);

	void executeBeforeProcessPageRequests(LinkedTask& task)
	{
		messageQueue.push(&task);
	}
};