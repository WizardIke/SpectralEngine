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
		VirtualTextureInfo* textureInfo;
		ID3D12CommandQueue* commandQueue;
		ID3D12Device* graphicsDevice;
		void(*callback)(AllocateTextureRequest& unloadRequest, void* tr, void* gr);

		AllocateTextureRequest() {}
		AllocateTextureRequest(VirtualTextureInfo& textureInfo1, ID3D12CommandQueue& commandQueue1, ID3D12Device& graphicsDevice1,
			void(&callback1)(AllocateTextureRequest& unloadRequest, void* tr, void* gr)) :
			textureInfo(&textureInfo1),
			commandQueue(&commandQueue1),
			graphicsDevice(&graphicsDevice1),
			callback(callback1)
		{}
	};
private:
	PageCache pageCache;
	PageAllocator pageAllocator;
	VirtualTextureInfoByID texturesByID;
	std::unordered_map<PageResourceLocation, PageRequestData, PageResourceLocation::Hash> uniqueRequests;
	ResizingArray<std::pair<PageResourceLocation, unsigned long long>> posableLoadRequests;
	PageLoadRequest pageLoadRequests[maxPagesLoading];
	std::size_t freePageLoadRequestsCount;
	PageLoadRequest* freePageLoadRequests;
	UnorderedMultiProducerSingleConsumerQueue messageQueue;
	UnorderedMultiProducerSingleConsumerQueue halfFinishedPageLoadRequests; //page is in cpu memory waiting to be copied to gpu memory

	static void copyPageToUploadBuffer(StreamingManager::StreamingRequest* useSubresourceRequest, const unsigned char* data);
	static void addPageLoadRequestHelper(PageLoadRequest& pageRequest,
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
		void(*resourceUploaded)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources));

	template<class ThreadResources, class GlobalResources>
	static void addPageLoadRequest(PageLoadRequest& pageRequest, ThreadResources& threadResources, GlobalResources&)
	{
		threadResources.taskShedular.pushBackgroundTask({ &pageRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& sharedResources)
		{
			PageLoadRequest& pageRequest = *static_cast<PageLoadRequest*>(requester);
			addPageLoadRequestHelper(pageRequest, [](StreamingManager::StreamingRequest* request, void* tr, void* gr)
			{
				PageLoadRequest& uploadRequest = *static_cast<PageLoadRequest*>(request);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

				uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& req, void* tr, void* gr, const unsigned char* buffer)
				{
					PageLoadRequest& request = static_cast<PageLoadRequest&>(req);
					ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
					GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
					AsynchronousFileManager& asynchronousFileManager = globalResources.asynchronousFileManager;

					copyPageToUploadBuffer(&request, buffer);
					request.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& req, void*, void* gr)
					{
						PageLoadRequest& request = static_cast<PageLoadRequest&>(req);
						GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
						PageProvider& pageProvider = globalResources.virtualTextureManager.pageProvider;

						pageProvider.halfFinishedPageLoadRequests.push(&static_cast<LinkedTask&>(request));
					};
					asynchronousFileManager.discard(&request, threadResources, globalResources);
				};
				globalResources.asynchronousFileManager.readFile(&uploadRequest, threadResources, globalResources);
			}, [](StreamingManager::StreamingRequest* requester, void*, void* gr)
			{
				PageLoadRequest& request = static_cast<PageLoadRequest&>(*requester);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				PageProvider& pageProvider = globalResources.virtualTextureManager.pageProvider;

				request.execute = [](LinkedTask& task, void*, void*)
				{
					PageLoadRequest& pageRequest = static_cast<PageLoadRequest&>(task);
					PageProvider& pageProvider = *pageRequest.pageProvider;
					++pageProvider.freePageLoadRequestsCount;
					static_cast<LinkedTask&>(pageRequest).next = static_cast<LinkedTask*>(pageProvider.freePageLoadRequests);
					pageProvider.freePageLoadRequests = &pageRequest;
				};
				pageProvider.messageQueue.push(static_cast<LinkedTask*>(&request));
			});
			StreamingManager& streamingManager = sharedResources.streamingManager;
			streamingManager.addUploadRequest(&pageRequest, threadResources, sharedResources);
		} });
	}

	template<class ThreadResources, class GlobalResources>
	void startLoadingRequiredPages(ThreadResources& threadResources, GlobalResources& globalResources, VirtualTextureInfoByID& textureInfoById, PageDeleter& pageDeleter)
	{
		if (!posableLoadRequests.empty())
		{
			for(const auto& requestInfo : posableLoadRequests)
			{
				PageLoadRequest& pageRequest = *freePageLoadRequests;
				freePageLoadRequests = static_cast<PageLoadRequest*>(static_cast<LinkedTask*>(static_cast<LinkedTask*>(freePageLoadRequests)->next));
				pageRequest.allocationInfo.textureLocation = requestInfo.first;
				addPageLoadRequest(pageRequest, threadResources, globalResources);
				VirtualTextureInfo& textureInfo = textureInfoById[requestInfo.first.textureId];
				pageCache.addNonAllocatedPage(requestInfo.first, textureInfo, textureInfoById, pageDeleter);
			}
			freePageLoadRequestsCount -= posableLoadRequests.size();
			posableLoadRequests.clear();
		}
	}

	void addNewPagesToResources(GraphicsEngine& graphicsEngine,
		ID3D12GraphicsCommandList* commandList, void(*uploadComplete)(LinkedTask& task, void* gr, void* tr), void* tr, void* gr);

	static void addPageDataToResource(VirtualTextureInfo& textureInfo, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, PageLoadRequest** pageLoadRequests, std::size_t pageCount,
		D3D12_TILE_REGION_SIZE& tileSize, PageCache& pageCache, ID3D12GraphicsCommandList* commandList, GraphicsEngine& graphicsEngine,
		void(*uploadComplete)(PrimaryTaskFromOtherThreadQueue::Task& task, void* gr, void* tr));

	static void checkCacheForPage(std::pair<const PageResourceLocation, PageRequestData>& request, VirtualTextureInfoByID& texturesByID, PageCache& pageCache,
		ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests);
	static void checkCacheForPages(decltype(uniqueRequests)& pageRequests, VirtualTextureInfoByID& texturesByID, PageCache& pageCache,
		ResizingArray<std::pair<PageResourceLocation, unsigned long long>>& posableLoadRequests);
	void shrinkNumberOfLoadRequestsIfNeeded(std::size_t numTexturePagesThatCanBeRequested);
	void processMessages(void* tr, void* gr);
	bool recalculateCacheSize(float& mipBias, float desiredMipBias, long long maxPages, std::size_t totalPagesNeeded, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter);
	long long calculateMemoryBudgetInPages(IDXGIAdapter3* adapter);
	void processPageRequestsHelper(IDXGIAdapter3* adapter, float& mipBias, float desiredMipBias, PageDeleter& pageDeleter, void* tr, void* gr);
	void increaseMipBias(decltype(uniqueRequests)& pageRequests);

	void deleteTextureHelper(UnloadRequest& unloadRequest, void* tr, void* gr);
	void allocateTexturePinnedHelper(AllocateTextureRequest& allocateRequest, void* tr, void* gr);
	void allocateTexturePackedHelper(AllocateTextureRequest& allocateRequest, void* tr, void* gr);
public:
	PageProvider();

	void gatherPageRequests(void* feadBackBuffer, unsigned long sizeInBytes);

	template<class ThreadResources, class GlobalResources>
	void processPageRequests(ThreadResources& threadResources, GlobalResources& globalResources, float& mipBias, float desiredMipBias)
	{
		PageDeleter pageDeleter(pageAllocator, globalResources.graphicsEngine.directCommandQueue);
		processPageRequestsHelper(globalResources.graphicsEngine.adapter, mipBias, desiredMipBias, pageDeleter, &threadResources, &globalResources);
		startLoadingRequiredPages(threadResources, globalResources, texturesByID, pageDeleter);
		pageDeleter.finish(texturesByID);

		execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr, void* gr)
		{
			PageProvider& pageProvider = static_cast<PageProvider&>(task);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

			//we have finished with the readback resources so we can render again
			VirtualFeedbackSubPass& feedbackAnalizerSubPass = globalResources.renderPass.virtualTextureFeedbackSubPass();
			feedbackAnalizerSubPass.setInView();

			threadResources.taskShedular.pushPrimaryTask(1u, {&pageProvider, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				PageProvider& pageProvider = *static_cast<PageProvider*>(requester);
				pageProvider.addNewPagesToResources(globalResources.graphicsEngine,
					threadResources.renderPass.virtualTextureFeedbackSubPass().firstCommandList(), [](LinkedTask& task, void* tr, void* gr)
				{
					PageLoadRequest& request = static_cast<PageLoadRequest&>(task);
					ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
					GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
					StreamingManager& streamingManager = globalResources.streamingManager;
					streamingManager.uploadFinished(&request, threadResources, globalResources);
				}, &threadResources, &globalResources);
			}});
		};
		globalResources.taskShedular.pushPrimaryTaskFromOtherThread(0u, *this);
	}

	void deleteTexture(UnloadRequest& unloadRequest);
	void allocateTexturePinned(AllocateTextureRequest& allocateRequest);
	void allocateTexturePacked(AllocateTextureRequest& allocateRequest);
};