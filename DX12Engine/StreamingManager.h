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

	void addUploadToBuffer(BaseExecutor* const executor, SharedResources& sharedResources);
	void backgroundUpdate(BaseExecutor* const executor, SharedResources& sharedResources);
public:
	StreamingManager(ID3D12Device* const graphicsDevice, unsigned long uploadHeapStartingSize);
	void update(BaseExecutor* const executor, SharedResources& sharedResources);
	bool hasPendingUploads();
	ID3D12CommandQueue* commandQueue() { return copyCommandQueue; }
	void addUploadRequest(const RamToVramUploadRequest& request);
	void addCopyCompletionEvent(BaseExecutor* executor, void* requester, void(*event)(void* requester, BaseExecutor* executor, SharedResources& sharedResources));

	class ThreadLocal
	{
		D3D12FencePointer copyFence;
		std::atomic<uint64_t> mFenceValue;
		Array<D3D12CommandAllocator, 2u> commandAllocators;
		D3D12GraphicsCommandList commandLists[2u];
		ID3D12GraphicsCommandList* currentCommandList; //Need to handle swapping currentCommandList in a thread safe manner
	public:
		ThreadLocal(ID3D12Device* const graphicsDevice);
		ID3D12GraphicsCommandList * copyCommandList() { return currentCommandList; }
		void update(BaseExecutor* const executor, SharedResources& sharedResources);
		void copyStarted(BaseExecutor* const executor, HalfFinishedUploadRequest& processingRequest);
		uint64_t fenceValue() { return mFenceValue.load(std::memory_order::memory_order_relaxed); }
	};
};