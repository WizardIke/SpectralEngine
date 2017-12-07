#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "BaseExecutor.h"
#include "SharedResources.h"

static void resizeBuffer(std::unique_ptr<RamToVramUploadRequest[]>& buffer, unsigned int& readPos, unsigned int& writePos, unsigned int& capacity)
{
	unsigned int newCapacity = static_cast<unsigned int>(capacity * 1.2f);
	RamToVramUploadRequest* tempBuffer = new RamToVramUploadRequest[newCapacity];

	unsigned int oldReadPos = readPos;
	readPos = 0;
	unsigned int cap = capacity;
	writePos = cap;

	RamToVramUploadRequest* data = buffer.get();
	for (unsigned int i = 0u; i < cap; ++i) {
		tempBuffer[i] = std::move(data[oldReadPos]);
		++oldReadPos;
		if (oldReadPos == cap) oldReadPos = 0u;
	}
	buffer.reset(tempBuffer);
	capacity = newCapacity;
}

StreamingManager::StreamingManager(ID3D12Device* const graphicsDevice) :
	copyCommandQueue(graphicsDevice, []()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0u;
	return commandQueueDesc;
}())
{
#ifndef NDEBUG
	copyCommandQueue->SetName(L"Streaming copy command queue");
#endif // NDEBUG
}

void StreamingManagerThreadLocal::addUploadToBuffer(BaseExecutor* const executor)
{
	auto readPos = uploadRequestBufferReadPos;
	while (readPos != uploadRequestBufferWritePos)
	{
		auto& uploadRequest = uploadRequestBuffer[readPos];
		auto numSubresources = uploadRequest.numSubresources;
		while (uploadRequest.currentSubresourceIndex < numSubresources)
		{
			auto startWritePos = uploadBufferWritePos;

			size_t numBytes, numRows, rowBytes;
			size_t subresouceWidth = uploadRequest.width >> uploadRequest.currentSubresourceIndex;
			if (subresouceWidth == 0u) subresouceWidth = 1u;
			size_t subresourceHeight = uploadRequest.height >> uploadRequest.currentSubresourceIndex;
			if (subresourceHeight == 0u) subresourceHeight = 1u;
			size_t subresourceDepth = uploadRequest.depth >> uploadRequest.currentSubresourceIndex;
			if (subresourceDepth == 0u) subresourceDepth = 1u;

			size_t subresourceSize;
			if (uploadRequest.format != DXGI_FORMAT_UNKNOWN)
			{
				DDSFileLoader::getSurfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.format, numBytes, rowBytes, numRows);
				subresourceSize = numBytes * subresourceDepth;
			}
			else //we have a buffer
			{
				subresourceSize = subresouceWidth;
			}


			unsigned long newUploadBufferWritePos = (uploadBufferWritePos + (unsigned long)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - (unsigned long)1u) & ~((unsigned long)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - (unsigned long)1u);

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

				uploadRequest.useSubresource(executor, reinterpret_cast<void*>(uploadBufferCpuAddress + newUploadBufferWritePos), uploadBuffer, newUploadBufferWritePos);
				uploadBufferWritePos = requiredWriteIndex;

				++(uploadRequest.currentSubresourceIndex);
				//create event to free memory when gpu finishes
				currentHalfFinishedUploadRequestBuffer->push_back(HalfFinishedUploadRequest{ requiredWriteIndex - startWritePos, uploadRequest.requester,
					uploadRequest.currentSubresourceIndex == uploadRequest.numSubresources ? uploadRequest.resourceUploadedPointer : nullptr });
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

						uploadBuffer = D3D12Resource(executor->sharedResources->graphicsEngine.graphicsDevice, uploadHeapProperties, D3D12_HEAP_FLAG_NONE, uploadResouceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

						D3D12_RANGE readRange = { 0u, 0u };
						uploadBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&uploadBufferCpuAddress));
					}
				}
				return;
			}
		}

		++readPos;
		if (readPos == uploadRequestBufferCapacity) readPos = 0u;
		uploadRequestBufferReadPos = readPos;
	}
}

StreamingManagerThreadLocal::StreamingManagerThreadLocal(ID3D12Device* const graphicsDevice, unsigned long uploadHeapStartingSize, unsigned int uploadRequestBufferStartingCapacity, unsigned int halfFinishedUploadRequestBufferStartingCapasity) :
	commandAllocators([graphicsDevice](size_t i, D3D12CommandAllocator& allocator)
{
	new(&allocator) D3D12CommandAllocator(graphicsDevice, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY);
}),
	fenceValue{ 0u },
	copyFence(graphicsDevice, 0u, D3D12_FENCE_FLAG_NONE),
	uploadRequestBuffer(new RamToVramUploadRequest[uploadRequestBufferStartingCapacity]),
	uploadRequestBufferCapacity(uploadRequestBufferStartingCapacity),
	uploadBufferCapasity(uploadHeapStartingSize),
	halfFinishedUploadRequestsBuffer([=](size_t i, std::vector<HalfFinishedUploadRequest>& element)
{
	new(&element) std::vector<HalfFinishedUploadRequest>();
	element.reserve(halfFinishedUploadRequestBufferStartingCapasity);
}),
	currentHalfFinishedUploadRequestBuffer(&halfFinishedUploadRequestsBuffer[0u])
{
	new(&commandLists[1u]) D3D12GraphicsCommandListPointer(graphicsDevice, 0u, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, commandAllocators[0u], nullptr);
	commandLists[1u]->Close();
	new(&commandLists[0u]) D3D12GraphicsCommandListPointer(graphicsDevice, 0u, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, commandAllocators[0u], nullptr);
	currentCommandList = commandLists[0u];

#ifndef NDEBUG
	commandLists[1u]->SetName(L"Streaming copy command list 1");
	commandLists[0u]->SetName(L"Streaming copy command list 0");
	commandAllocators[1u]->SetName(L"Streaming copy command allocator 1");
	commandAllocators[0u]->SetName(L"Streaming copy command allocator 0");
#endif // NDEBUG

	if (uploadHeapStartingSize != 0u)
	{
		D3D12_HEAP_PROPERTIES uploadHeapProperties;
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadHeapProperties.CreationNodeMask = 0u;
		uploadHeapProperties.VisibleNodeMask = 0u;

		D3D12_RESOURCE_DESC uploadResouceDesc;
		uploadResouceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadResouceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		uploadResouceDesc.Width = uploadHeapStartingSize;
		uploadResouceDesc.Height = 1u;
		uploadResouceDesc.DepthOrArraySize = 1u;
		uploadResouceDesc.MipLevels = 1u;
		uploadResouceDesc.Format = DXGI_FORMAT_UNKNOWN;
		uploadResouceDesc.SampleDesc.Count = 1u;
		uploadResouceDesc.SampleDesc.Quality = 0u;
		uploadResouceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		uploadResouceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		new(&uploadBuffer) D3D12Resource(graphicsDevice, uploadHeapProperties, D3D12_HEAP_FLAG_NONE, uploadResouceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

		D3D12_RANGE Range = { 0u, 0u };
		uploadBuffer->Map(0u, &Range, reinterpret_cast<void**>(&uploadBufferCpuAddress));

#ifndef NDEBUG
		uploadBuffer->SetName(L"Streaming upload buffer");
#endif // NDEBUG

	}
}

void StreamingManagerThreadLocal::update(BaseExecutor* const executor)
{
	auto& manager = executor->sharedResources->streamingManager;
	if (copyFence->GetCompletedValue() >= fenceValue)
	{
		auto hr = currentCommandList->Close();
		assert(hr == S_OK);
		ID3D12CommandList* lists = currentCommandList;
		manager.copyCommandQueue->ExecuteCommandLists(1u, &lists);
		++fenceValue;
		hr = manager.copyCommandQueue->Signal(copyFence, fenceValue);
		assert(hr == S_OK);

		ID3D12CommandAllocator* currentCommandAllocator;
		if (currentCommandList == commandLists[0u])
		{
			currentCommandList = commandLists[1u];
			currentHalfFinishedUploadRequestBuffer = &halfFinishedUploadRequestsBuffer[1u];
			currentCommandAllocator = commandAllocators[1u].get();
		}
		else
		{
			currentCommandList = commandLists[0u];
			currentHalfFinishedUploadRequestBuffer = &halfFinishedUploadRequestsBuffer[0u];
			currentCommandAllocator = commandAllocators[0u].get();
		}

		hr = currentCommandAllocator->Reset();
		assert(hr == S_OK);

		currentCommandList->Reset(currentCommandAllocator, nullptr);

		auto readPos = uploadBufferReadPos;
		for (auto& halfFinishedUploadRequest : *currentHalfFinishedUploadRequestBuffer)
		{
			halfFinishedUploadRequest.subresourceUploaded(executor);
			readPos += halfFinishedUploadRequest.numberOfBytesToFree; //free memory
		}
		uploadBufferReadPos = readPos;
		currentHalfFinishedUploadRequestBuffer->clear();
	}

	//load resources into the upload buffer if there is enough space
	addUploadToBuffer(executor);
}

RamToVramUploadRequest& StreamingManagerThreadLocal::getUploadRequest()
{
	RamToVramUploadRequest& returnValue = uploadRequestBuffer[uploadRequestBufferWritePos];
	++uploadRequestBufferWritePos;
	if (uploadRequestBufferWritePos == uploadRequestBufferCapacity) uploadRequestBufferWritePos = 0u;
	if (uploadRequestBufferWritePos == uploadRequestBufferReadPos)
	{
		resizeBuffer(uploadRequestBuffer, uploadRequestBufferReadPos, uploadRequestBufferWritePos, uploadRequestBufferCapacity);
	}
	returnValue.currentSubresourceIndex = 0u;
	return returnValue;
}