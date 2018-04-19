#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "BaseExecutor.h"
#include "SharedResources.h"


StreamingManager::StreamingManager(ID3D12Device* const graphicsDevice, unsigned long uploadHeapStartingSize) :
	copyCommandQueue(graphicsDevice, []()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0u;
	return commandQueueDesc;
}()),
	waitingForSpaceRequestBuffer(),
	processingRequestBuffer(),
	uploadBufferCapasity(uploadHeapStartingSize)
{
	if (uploadHeapStartingSize != 0u)
	{
		D3D12_HEAP_PROPERTIES uploadHeapProperties;
		uploadHeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadHeapProperties.CreationNodeMask = 0u;
		uploadHeapProperties.VisibleNodeMask = 0u;

		D3D12_RESOURCE_DESC uploadResouceDesc;
		uploadResouceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadResouceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		uploadResouceDesc.Width = uploadHeapStartingSize;
		uploadResouceDesc.Height = 1u;
		uploadResouceDesc.DepthOrArraySize = 1u;
		uploadResouceDesc.MipLevels = 1u;
		uploadResouceDesc.Format = DXGI_FORMAT_UNKNOWN;
		uploadResouceDesc.SampleDesc.Count = 1u;
		uploadResouceDesc.SampleDesc.Quality = 0u;
		uploadResouceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
#ifndef NDEBUG
		uploadResouceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE; //graphics debugger can't handle D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
#else
		uploadResouceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
#endif

		new(&uploadBuffer) D3D12Resource(graphicsDevice, uploadHeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			uploadResouceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

		D3D12_RANGE Range = { 0u, 0u };
		uploadBuffer->Map(0u, &Range, reinterpret_cast<void**>(&uploadBufferCpuAddress));

#ifndef NDEBUG
		uploadBuffer->SetName(L"Streaming upload buffer");
#endif // NDEBUG

	}
#ifndef NDEBUG
	copyCommandQueue->SetName(L"Streaming copy command queue");
#endif // NDEBUG
}

void StreamingManager::addUploadToBuffer(BaseExecutor* const executor, SharedResources& sharedResources)
{
	while (!waitingForSpaceRequestBuffer.empty())
	{
		auto& uploadRequest = waitingForSpaceRequestBuffer.front();
		while (uploadRequest.currentArrayIndex < uploadRequest.arraySize)
		{
			auto startWritePos = uploadBufferWritePos;

			size_t subresourceSize;
			if (uploadRequest.dimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				size_t subresouceWidth = uploadRequest.textureInfo.width >> uploadRequest.currentMipLevel;
				if (subresouceWidth == 0u) subresouceWidth = 1u;
				size_t subresourceHeight = uploadRequest.textureInfo.height >> uploadRequest.currentMipLevel;
				if (subresourceHeight == 0u) subresourceHeight = 1u;
				size_t subresourceDepth = uploadRequest.textureInfo.depth >> uploadRequest.currentMipLevel;
				if (subresourceDepth == 0u) subresourceDepth = 1u;

				size_t numBytes, numRows, rowBytes;
				DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.textureInfo.format, numBytes, rowBytes, numRows);
				subresourceSize = numBytes * subresourceDepth;
			}
			else
			{
				subresourceSize = uploadRequest.meshInfo.sizeInBytes;
			}

			unsigned long newUploadBufferWritePos = (uploadBufferWritePos + (unsigned long)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - (unsigned long)1u) & 
				~((unsigned long)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - (unsigned long)1u);

			unsigned long requiredWriteIndex = newUploadBufferWritePos + static_cast<unsigned long>(subresourceSize);
			if ((newUploadBufferWritePos < uploadBufferReadPos && requiredWriteIndex < uploadBufferReadPos) ||
				(newUploadBufferWritePos >= uploadBufferReadPos && requiredWriteIndex < uploadBufferCapasity) ||
				(newUploadBufferWritePos >= uploadBufferReadPos && subresourceSize < uploadBufferReadPos))
			{
				if (requiredWriteIndex > uploadBufferCapasity)
				{
					newUploadBufferWritePos = 0u;
					requiredWriteIndex = static_cast<unsigned long>(subresourceSize);
				}

				processingRequestBuffer.emplace_back();
				HalfFinishedUploadRequest& processingRequest = processingRequestBuffer.back();
				processingRequest.copyFenceValue.store(std::numeric_limits<uint64_t>::max() - 2u, std::memory_order::memory_order_relaxed); //copy not finished
				processingRequest.numberOfBytesToFree = requiredWriteIndex - startWritePos;
				processingRequest.requester = uploadRequest.requester;
				processingRequest.resourceUploadedPointer = uploadRequest.currentArrayIndex == uploadRequest.arraySize ? uploadRequest.resourceUploadedPointer : nullptr;
				processingRequest.uploadBufferCpuAddressOfCurrentPos = reinterpret_cast<void*>(uploadBufferCpuAddress + newUploadBufferWritePos);
				processingRequest.uploadResourceOffset = newUploadBufferWritePos;
				processingRequest.uploadRequest = &uploadRequest;
				processingRequest.currentArrayIndex = uploadRequest.currentArrayIndex;
				processingRequest.currentMipLevel = uploadRequest.currentMipLevel;

				processingRequest.useSubresource(executor, sharedResources);

				uploadBufferWritePos = requiredWriteIndex;
				++(uploadRequest.currentMipLevel);
				if (uploadRequest.currentMipLevel == uploadRequest.mipLevels)
				{
					uploadRequest.currentMipLevel = uploadRequest.mostDetailedMip;
					++uploadRequest.currentArrayIndex;
				}
			}
			else
			{
				if (uploadBufferReadPos == uploadBufferWritePos) //the buffer is empty
				{
					if (subresourceSize <= uploadBufferCapasity)
					{
						uploadBufferReadPos = 0u;
						uploadBufferWritePos = 0u;
					}
					else
					{
						//allocate a bigger buffer
						D3D12_RANGE writtenRange{ 0u, uploadBufferCapasity };
						uploadBuffer->Unmap(0u, &writtenRange);

						uploadBufferCapasity += uploadBufferCapasity >> 1u;

						D3D12_HEAP_PROPERTIES uploadHeapProperties;
						uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
						uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						uploadHeapProperties.CreationNodeMask = 0u;
						uploadHeapProperties.VisibleNodeMask = 0u;

						D3D12_RESOURCE_DESC uploadResouceDesc;
						uploadResouceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						uploadResouceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
						uploadResouceDesc.Width = uploadBufferCapasity;
						uploadResouceDesc.Height = 1u;
						uploadResouceDesc.DepthOrArraySize = 1u;
						uploadResouceDesc.MipLevels = 1u;
						uploadResouceDesc.Format = DXGI_FORMAT_UNKNOWN;
						uploadResouceDesc.SampleDesc.Count = 1u;
						uploadResouceDesc.SampleDesc.Quality = 0u;
						uploadResouceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
						uploadResouceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

						uploadBuffer = D3D12Resource(sharedResources.graphicsEngine.graphicsDevice, uploadHeapProperties, D3D12_HEAP_FLAG_NONE, uploadResouceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

						D3D12_RANGE readRange = { 0u, 0u };
						uploadBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&uploadBufferCpuAddress));
					}
				}
				return;
			}
		}

		waitingForSpaceRequestBuffer.pop_front();
	}
}

void StreamingManager::update(BaseExecutor* const executor, SharedResources& sharedResources)
{
	if (finishedUpdating.load(std::memory_order::memory_order_acquire))
	{
		finishedUpdating.store(false, std::memory_order::memory_order_relaxed);
		sharedResources.backgroundQueue.push(Job(this, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			reinterpret_cast<StreamingManager*>(requester)->backgroundUpdate(executor, sharedResources);
		}));
	}
}


void StreamingManager::backgroundUpdate(BaseExecutor* const executor, SharedResources& sharedResources)
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	
	auto readPos = uploadBufferReadPos;
	while (!processingRequestBuffer.empty())
	{
		HalfFinishedUploadRequest& processingRequest = processingRequestBuffer.front();
		//fenceValue must be incremented twice. Once when the copy commands are submited to the GPU and once when the commands are finished on the GPU (The next commands are submited).
		auto requestCopyFenceValue = processingRequest.copyFenceValue.load(std::memory_order::memory_order_acquire);
		if (requestCopyFenceValue + 2u > sharedResources.executors[executor->index]->streamingManager.fenceValue()) break;
		readPos += processingRequest.numberOfBytesToFree; //free CPU memory
		processingRequestBuffer.pop_front();
	}
	uploadBufferReadPos = readPos;

	//load resources into the upload buffer if there is enough space
	addUploadToBuffer(executor, sharedResources);

	finishedUpdating.store(true, std::memory_order::memory_order_release);
}


void StreamingManager::addUploadRequest(const RamToVramUploadRequest& request)
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	waitingForSpaceRequestBuffer.push_back(request);
	waitingForSpaceRequestBuffer.back().uploadResource = uploadBuffer;
}

void StreamingManager::addCopyCompletionEvent(BaseExecutor* executor, void* requester, void(*event)(void* requester, BaseExecutor* executor, SharedResources& sharedResources))
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	processingRequestBuffer.emplace_back();
	HalfFinishedUploadRequest& processingRequest = processingRequestBuffer.back();
	processingRequest.copyFenceIndex = executor->index;
	processingRequest.copyFenceValue = executor->streamingManager.fenceValue();
	processingRequest.numberOfBytesToFree = 0u;
	processingRequest.requester = requester;
	processingRequest.resourceUploadedPointer = event;

	//processingRequest.uploadBufferCpuAddressOfCurrentPos = reinterpret_cast<void*>(uploadBufferCpuAddress + newUploadBufferWritePos);
	//processingRequest.uploadResourceOffset = newUploadBufferWritePos;
	//processingRequest.uploadRequest = &uploadRequest;
}

bool StreamingManager::hasPendingUploads()
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	return !waitingForSpaceRequestBuffer.empty() || !processingRequestBuffer.empty(); 
}

StreamingManager::ThreadLocal::ThreadLocal(ID3D12Device* const graphicsDevice) :
	commandAllocators([graphicsDevice](size_t i, D3D12CommandAllocator& allocator)
{
	new(&allocator) D3D12CommandAllocator(graphicsDevice, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY);
}),
	mFenceValue{ 0u },
	copyFence(graphicsDevice, 0u, D3D12_FENCE_FLAG_NONE)
{
	new(&commandLists[1u]) D3D12GraphicsCommandList(graphicsDevice, 0u, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, commandAllocators[0u], nullptr);
	commandLists[1u]->Close();
	new(&commandLists[0u]) D3D12GraphicsCommandList(graphicsDevice, 0u, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, commandAllocators[0u], nullptr);
	currentCommandList = commandLists[0u];

#ifndef NDEBUG
	commandLists[1u]->SetName(L"Streaming copy command list 1");
	commandLists[0u]->SetName(L"Streaming copy command list 0");
	commandAllocators[1u]->SetName(L"Streaming copy command allocator 1");
	commandAllocators[0u]->SetName(L"Streaming copy command allocator 0");
#endif // NDEBUG
}

void StreamingManager::ThreadLocal::update(BaseExecutor* const executor, SharedResources& sharedResources)
{
	if (copyFence->GetCompletedValue() == mFenceValue)
	{
		auto& manager = sharedResources.streamingManager;
		auto hr = currentCommandList->Close();
		assert(hr == S_OK);
		ID3D12CommandList* lists = currentCommandList;
		manager.copyCommandQueue->ExecuteCommandLists(1u, &lists);
		++mFenceValue;
		hr = manager.copyCommandQueue->Signal(copyFence, mFenceValue);
		assert(hr == S_OK);

		ID3D12CommandAllocator* currentCommandAllocator;
		if (currentCommandList == commandLists[0u])
		{
			currentCommandList = commandLists[1u];
			currentCommandAllocator = commandAllocators[1u].get();
		}
		else
		{
			currentCommandList = commandLists[0u];
			currentCommandAllocator = commandAllocators[0u].get();
		}

		hr = currentCommandAllocator->Reset();
		assert(hr == S_OK);

		currentCommandList->Reset(currentCommandAllocator, nullptr);
	}
}

void StreamingManager::ThreadLocal::copyStarted(BaseExecutor* const executor, HalfFinishedUploadRequest& processingRequest)
{
	processingRequest.copyFenceIndex = executor->index;
	processingRequest.copyFenceValue.store(mFenceValue, std::memory_order::memory_order_release);
}