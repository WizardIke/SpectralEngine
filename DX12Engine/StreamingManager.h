#pragma once
#include "D3D12CommandQueue.h"
#include "D3D12Fence.h"
#include "../Array/Array.h"
#include "D3D12CommandAllocator.h"
#include "D3D12GraphicsCommandList.h"
#include <memory>
#include <cassert>
#include <vector>
#include "RamToVramUploadRequest.h"
#include "HalfFinishedUploadRequset.h"
#include "D3D12Resource.h"

class StreamingManager
{
	friend class StreamingManagerThreadLocal;
	D3D12CommandQueue copyCommandQueue;
public:
	StreamingManager(ID3D12Device* const graphicsDevice);
	ID3D12CommandQueue* commandQueue()
	{
		return copyCommandQueue;
	}
};

class StreamingManagerThreadLocal
{
	D3D12FencePointer copyFence;
	uint64_t fenceValue;
	Array<D3D12CommandAllocator, 2u> commandAllocators;
	D3D12GraphicsCommandList commandLists[2u];

	std::unique_ptr<RamToVramUploadRequest[]> uploadRequestBuffer;
	unsigned int uploadRequestBufferCapacity;
	unsigned int uploadRequestBufferWritePos = 0u;
	unsigned int uploadRequestBufferReadPos = 0u;

	Array<std::vector<HalfFinishedUploadRequest>, 2u> halfFinishedUploadRequestsBuffer;
	std::vector<HalfFinishedUploadRequest>* currentHalfFinishedUploadRequestBuffer;

	D3D12Resource uploadBuffer;
	unsigned char* uploadBufferCpuAddress;
	unsigned long uploadBufferCapasity;
	unsigned long uploadBufferWritePos = 0u;
	unsigned long uploadBufferReadPos = 0u;

	void addUploadToBuffer(BaseExecutor* const executor);
public:
	StreamingManagerThreadLocal(ID3D12Device* const graphicsDevice, unsigned long uploadHeapStartingSize, unsigned int uploadRequestBufferStartingCapacity, unsigned int halfFinishedUploadRequestBufferStartingCapasity);

	void update(BaseExecutor* const executor);
	RamToVramUploadRequest& getUploadRequest();


	bool hasPendingUploads() { return uploadRequestBufferWritePos != uploadRequestBufferReadPos; }

	ID3D12GraphicsCommandList* currentCommandList;
};