#include "StreamingManager.h"
#include "DDSFileLoader.h"

StreamingManager::StreamingManager(ID3D12Device& graphicsDevice, unsigned long uploadHeapStartingSize) :
	copyCommandQueue(&graphicsDevice, []()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0u;
	return commandQueueDesc;
}()),
	processingRequestsStart(nullptr),
	processingRequestsEnd(nullptr),
	uploadBufferCapasity(uploadHeapStartingSize),
	uploadBuffer(&graphicsDevice, []()
{
	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 0u;
	uploadHeapProperties.VisibleNodeMask = 0u;
	return uploadHeapProperties;
}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [uploadHeapStartingSize]()
{
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
	return uploadResouceDesc;
}(), D3D12_RESOURCE_STATE_GENERIC_READ)
{
	D3D12_RANGE Range = { 0u, 0u };
	uploadBuffer->Map(0u, &Range, reinterpret_cast<void**>(&uploadBufferCpuAddress));

#ifndef NDEBUG
	uploadBuffer->SetName(L"Streaming upload buffer");
	copyCommandQueue->SetName(L"Streaming copy command queue");
#endif // NDEBUG
}

void StreamingManager::run(ID3D12Device& device, void* threadResources, void* globalResources)
{
	do
	{
		bool hasSpaceToFree = false;
		SinglyLinked* temp = messageQueue.popAll();
		for(; temp != nullptr; temp = temp->next)
		{
			auto& message = *static_cast<StreamingRequest*>(temp);
			if(message.action == Action::deallocate)
			{
				message.readyToDelete = true;
				hasSpaceToFree = true;
			}
			else
			{
				message.readyToDelete = false;
				if(waitingForSpaceRequestsStart == nullptr)
				{
					waitingForSpaceRequestsStart = &message;
				}
				else
				{
					waitingForSpaceRequestsEnd->next = &message;
				}
				waitingForSpaceRequestsEnd = &message;
			}
		}
		waitingForSpaceRequestsEnd->next = nullptr;
		if(hasSpaceToFree)
		{
			freeSpace(threadResources, globalResources);
		}

		//load resources into the upload buffer if there is enough space
		allocateSpace(device, threadResources, globalResources);
	} while(!messageQueue.stop());
}

void StreamingManager::freeSpace(void* threadResources, void* globalResources)
{
	auto readPos = uploadBufferReadPos;
	while(processingRequestsStart != nullptr)
	{
		StreamingRequest& request = *processingRequestsStart;
		if(!request.readyToDelete) break;
		readPos += request.numberOfBytesToFree; //free CPU memory
		processingRequestsStart = processingRequestsStart->nextToDelete; //Need to do this now as the next line will delete request
		request.resourceUploaded(&request, threadResources, globalResources);
	}
	uploadBufferReadPos = readPos;
}

void StreamingManager::allocateSpace(ID3D12Device& graphicsDevice, void* executor, void* sharedResources)
{
	while(waitingForSpaceRequestsStart != nullptr)
	{
		StreamingRequest& uploadRequest = *waitingForSpaceRequestsStart;
		const unsigned long resourceSize = uploadRequest.resourceSize;

		auto startWritePos = uploadBufferWritePos;
		unsigned long newUploadBufferWritePos = (uploadBufferWritePos + (unsigned long)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - (unsigned long)1u) &
			~((unsigned long)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - (unsigned long)1u);

		unsigned long requiredWriteIndex = newUploadBufferWritePos + resourceSize;
		if((newUploadBufferWritePos < uploadBufferReadPos && requiredWriteIndex < uploadBufferReadPos) ||
			(newUploadBufferWritePos >= uploadBufferReadPos && requiredWriteIndex < uploadBufferCapasity) ||
			(newUploadBufferWritePos >= uploadBufferReadPos && resourceSize < uploadBufferReadPos))
		{
			if(requiredWriteIndex > uploadBufferCapasity)
			{
				newUploadBufferWritePos = 0u;
				requiredWriteIndex = resourceSize;
			}

			uploadRequest.nextToDelete = nullptr;
			if(processingRequestsStart == nullptr)
			{
				processingRequestsStart = &uploadRequest;
			}
			else
			{
				processingRequestsEnd->nextToDelete = &uploadRequest;
			}
			processingRequestsEnd = &uploadRequest;
			
			uploadRequest.numberOfBytesToFree = requiredWriteIndex - startWritePos;
			uploadRequest.uploadBufferCurrentCpuAddress = uploadBufferCpuAddress + newUploadBufferWritePos;
			uploadRequest.uploadResourceOffset = newUploadBufferWritePos;
			uploadRequest.uploadResource = uploadBuffer;
			uploadRequest.streamResource(&uploadRequest, executor, sharedResources);

			uploadBufferWritePos = requiredWriteIndex;

			waitingForSpaceRequestsStart = static_cast<StreamingRequest*>(waitingForSpaceRequestsStart->next);
		}
		else
		{
			if(uploadBufferReadPos == uploadBufferWritePos) //the buffer is empty
			{
				if(resourceSize <= uploadBufferCapasity)
				{
					uploadBufferReadPos = 0u;
					uploadBufferWritePos = 0u;
				}
				else
				{
					increaseBufferCapacity(graphicsDevice);
				}
			}
			else
			{
				//Out of space, try again later.
				return;
			}
		}
	}
}

void StreamingManager::increaseBufferCapacity(ID3D12Device& graphicsDevice)
{
	D3D12_RANGE writtenRange{0u, uploadBufferCapasity};
	uploadBuffer->Unmap(0u, &writtenRange);

	uploadBufferCapasity <<= 1u;

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

	uploadBuffer = D3D12Resource(&graphicsDevice, uploadHeapProperties, D3D12_HEAP_FLAG_NONE, uploadResouceDesc, D3D12_RESOURCE_STATE_GENERIC_READ);

	D3D12_RANGE readRange = {0u, 0u};
	uploadBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&uploadBufferCpuAddress));
}

StreamingManager::ThreadLocal::ThreadLocal(ID3D12Device* const graphicsDevice) :
	commandAllocators([graphicsDevice](std::size_t, D3D12CommandAllocator& allocator)
{
	new(&allocator) D3D12CommandAllocator(graphicsDevice, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY);
}),
	fenceValue{ 0u },
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

void StreamingManager::ThreadLocal::update(StreamingManager& streamingManager, void* threadResources, void* globalResources)
{
	if (copyFence->GetCompletedValue() == fenceValue)
	{
		auto hr = currentCommandList->Close();
		assert(hr == S_OK);
		ID3D12CommandList* lists = currentCommandList;
		streamingManager.copyCommandQueue->ExecuteCommandLists(1u, &lists);
		++fenceValue;
		hr = streamingManager.copyCommandQueue->Signal(copyFence, fenceValue);
		assert(hr == S_OK);

		ID3D12CommandAllocator* currentCommandAllocator;
		std::size_t index;
		if (currentCommandList == commandLists[0u])
		{
			currentCommandList = commandLists[1u];
			currentCommandAllocator = commandAllocators[1u].get();
			index = 1u;
		}
		else
		{
			currentCommandList = commandLists[0u];
			currentCommandAllocator = commandAllocators[0u].get();
			index = 0u;
		}

		hr = currentCommandAllocator->Reset();
		assert(hr == S_OK);

		currentCommandList->Reset(currentCommandAllocator, nullptr);

		completionEventManager.update(index, threadResources, globalResources);
	}
}

void StreamingManager::ThreadLocal::addCopyCompletionEvent(void* requester, void(*unloadCallback)(void* const requester, void* executor, void* sharedResources))
{
	completionEventManager.addRequest(requester, unloadCallback, bufferIndex());
}