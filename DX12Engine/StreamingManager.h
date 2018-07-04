#pragma once
#include "D3D12CommandQueue.h"
#include "D3D12Fence.h"
#include "Array.h"
#include "D3D12CommandAllocator.h"
#include "D3D12GraphicsCommandList.h"
#include <memory>
#include <cassert>
#include "RamToVramUploadRequest.h"
#include "HalfFinishedUploadRequset.h"
#include "D3D12Resource.h"
#include <mutex>
#include <atomic>
#include "StableQueue.h"

class StreamingManager
{
public:
	class ThreadLocal
	{
		D3D12FencePointer copyFence;
		std::atomic<uint64_t> mFenceValue;
		Array<D3D12CommandAllocator, 2u> commandAllocators;
		D3D12GraphicsCommandList commandLists[2u];
		ID3D12GraphicsCommandList* currentCommandList; //Need to handle swapping currentCommandList in a thread safe manner
	public:
		ThreadLocal(ID3D12Device* const graphicsDevice, StreamingManager& streamingManager, unsigned int threadIndex);
		ID3D12GraphicsCommandList& copyCommandList() { return *currentCommandList; }
		void update(StreamingManager& streamingManager);
		void copyStarted(unsigned int threadIndex, HalfFinishedUploadRequest& processingRequest);
		uint64_t fenceValue() { return mFenceValue.load(std::memory_order::memory_order_relaxed); }
	};
private:
	std::mutex mutex;
	D3D12CommandQueue copyCommandQueue;

	StableQueue<RamToVramUploadRequest> waitingForSpaceRequestBuffer; //Needs to be a StableQueue so pointers into in other threads remain valid when it resizes.
	StableQueue<HalfFinishedUploadRequest> processingRequestBuffer; //Needs to be a StableQueue so pointers into in other threads remain valid when it resizes.

	D3D12Resource uploadBuffer;
	unsigned char* uploadBufferCpuAddress;
	unsigned long uploadBufferCapasity;
	unsigned long uploadBufferWritePos = 0u;
	unsigned long uploadBufferReadPos = 0u;

	std::atomic<bool> finishedUpdating = true;
	std::unique_ptr<ThreadLocal*[]> threadLocals;

	void addUploadToBuffer(ID3D12Device* graphicsDevice, void* executor, void* sharedResources);
	void backgroundUpdate(unsigned int threadIndex, ID3D12Device* device, void* threadResources, void* GlobalResources);
public:
	StreamingManager(ID3D12Device* const graphicsDevice, unsigned long uploadHeapStartingSize, unsigned int threadCount);

	template<class ThreadResources, class GlobalResources, class TaskShedularThreadLocal>
	void update(TaskShedularThreadLocal& taskShedular)
	{
		if (finishedUpdating.exchange(false, std::memory_order::memory_order_acquire))
		{
			taskShedular.backgroundQueue().push({ this, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				reinterpret_cast<StreamingManager*>(context)->backgroundUpdate(threadResources.taskShedular.index(), globalResources.graphicsEngine.graphicsDevice, &threadResources, &globalResources);
			} });
		}
	}

	ID3D12CommandQueue* commandQueue() { return copyCommandQueue; }
	void addUploadRequest(const RamToVramUploadRequest& request);
	void addCopyCompletionEvent(unsigned int threadIndex, void* requester, void(*event)(void* requester, void* executor, void* sharedResources));
};