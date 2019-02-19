#pragma once
#include "D3D12CommandQueue.h"
#include "D3D12Fence.h"
#include "Array.h"
#include "D3D12CommandAllocator.h"
#include "D3D12GraphicsCommandList.h"
#include <memory>
#include <cassert>
#include "D3D12Resource.h"
#include <mutex>
#include <atomic>
#include "GpuCompletionEventManager.h"
#include "ActorQueue.h"
#include "SinglyLinked.h"

class StreamingManager
{
	enum class Action : short
	{
		allocate,
		deallocate,
	};
public:
	class StreamingRequest : public SinglyLinked
	{
	public:
		StreamingRequest() {}

		void(*deleteStreamingRequest)(StreamingRequest* request, void* threadResources, void* globalResources);
		void(*streamResource)(StreamingRequest* request, void* threadResources, void* globalResources);
		unsigned long resourceSize;

		unsigned long uploadResourceOffset;
		ID3D12Resource* uploadResource;
		unsigned char* uploadBufferCurrentCpuAddress;
		unsigned long numberOfBytesToFree;

		StreamingRequest* nextToDelete;
		Action action;
		bool readyToDelete;
	};

	class ThreadLocal
	{
		D3D12Fence copyFence;
		uint64_t fenceValue;
		Array<D3D12CommandAllocator, 2u> commandAllocators;
		D3D12GraphicsCommandList commandLists[2u];
		ID3D12GraphicsCommandList* currentCommandList;
		GpuCompletionEventManager<2> completionEventManager;

		std::size_t bufferIndex() { return currentCommandList == commandLists[1] ? 1u : 0u; }
	public:
		ThreadLocal(ID3D12Device* const graphicsDevice);
		ID3D12GraphicsCommandList& copyCommandList() { return *currentCommandList; }
		void update(StreamingManager& streamingManager, void* threadResources, void* globalResources);
		void addCopyCompletionEvent(void* requester, void(*unloadCallback)(void* requester, void* executor, void* sharedResources));
	};
private:
	ActorQueue messageQueue;
	StreamingRequest* waitingForSpaceRequestsStart;
	StreamingRequest* waitingForSpaceRequestsEnd;
	StreamingRequest* processingRequestsStart;
	StreamingRequest* processingRequestsEnd;

	D3D12Resource uploadBuffer;
	unsigned char* uploadBufferCpuAddress;
	unsigned long uploadBufferCapasity;
	unsigned long uploadBufferWritePos = 0u;
	unsigned long uploadBufferReadPos = 0u;

	D3D12CommandQueue copyCommandQueue;

	void run(ID3D12Device& device, void* threadResources, void* globalResources);
	void freeSpace(void* threadResources, void* globalResources);
	void allocateSpace(ID3D12Device& graphicsDevice, void* executor, void* sharedResources);
	void increaseBufferCapacity(ID3D12Device& graphicsDevice);
public:
	StreamingManager(ID3D12Device& graphicsDevice, unsigned long uploadHeapStartingSize);

	template<class ThreadResources, class GlobalResources>
	void addUploadRequest(StreamingRequest* request, ThreadResources& threadResources, GlobalResources&)
	{
		request->action = Action::allocate;
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.backgroundQueue().push({this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& streamingManager = *static_cast<StreamingManager*>(requester);
				streamingManager.run(*globalResources.graphicsEngine.graphicsDevice, &threadResources, &globalResources);
			}});
		}
	}

	template<class ThreadResources, class GlobalResources>
	void uploadFinished(StreamingRequest* request, ThreadResources& threadResources, GlobalResources&)
	{
		request->action = Action::deallocate;
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.backgroundQueue().push({this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& streamingManager = *static_cast<StreamingManager*>(requester);
				streamingManager.run(*globalResources.graphicsEngine.graphicsDevice, &threadResources, &globalResources);
			}});
		}
	}

	ID3D12CommandQueue& commandQueue() { return *copyCommandQueue; }
};