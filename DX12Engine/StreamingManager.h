#pragma once
#include "D3D12CommandQueue.h"
#include "D3D12Fence.h"
#include <array>
#include "D3D12CommandAllocator.h"
#include "D3D12GraphicsCommandList.h"
#include <memory>
#include <cassert>
#include "D3D12Resource.h"
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

		void(*deleteStreamingRequest)(StreamingRequest* request, void* tr);
		void(*streamResource)(StreamingRequest* request, void* tr);
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
		std::array<D3D12CommandAllocator, 2u> commandAllocators;
		D3D12GraphicsCommandList commandLists[2u];
		ID3D12GraphicsCommandList* currentCommandList;
		GpuCompletionEventManager<2> completionEventManager;

		std::size_t bufferIndex() { return currentCommandList == commandLists[1] ? 1u : 0u; }

		friend class StreamingManager;
		void stop(StreamingManager& streamingManager, HANDLE fenceEvent);
	public:
		ThreadLocal(ID3D12Device& graphicsDevice);
		ID3D12GraphicsCommandList& copyCommandList() { return *currentCommandList; }
		void update(StreamingManager& streamingManager, void* tr);
		void addCopyCompletionEvent(void* requester, void(*unloadCallback)(void* requester, void* tr));
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
	ID3D12Device& graphicsDevice;

	void run(void* tr);
	void freeSpace(void* tr);
	void allocateSpace(void* tr);
	void increaseBufferCapacity();
public:
	StreamingManager(ID3D12Device& graphicsDevice, unsigned long uploadHeapStartingSize);

	template<class ThreadResources>
	void addUploadRequest(StreamingRequest* request, ThreadResources& threadResources)
	{
		request->action = Action::allocate;
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources)
			{
				auto& streamingManager = *static_cast<StreamingManager*>(requester);
				streamingManager.run(&threadResources);
			}});
		}
	}

	template<class ThreadResources>
	void uploadFinished(StreamingRequest* request, ThreadResources& threadResources)
	{
		request->action = Action::deallocate;
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources)
			{
				auto& streamingManager = *static_cast<StreamingManager*>(requester);
				streamingManager.run(&threadResources);
			}});
		}
	}

	ID3D12CommandQueue& commandQueue() { return *copyCommandQueue; }
	void stop(StreamingManager::ThreadLocal& local, HANDLE fenceEvent);
};