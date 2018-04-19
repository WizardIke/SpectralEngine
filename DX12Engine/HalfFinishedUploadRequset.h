#pragma once
struct ID3D12Fence;
class BaseExecutor;
class SharedResources;
#include <stdint.h>
#include <atomic>

class HalfFinishedUploadRequest
{
public:
	unsigned long numberOfBytesToFree;
	unsigned int copyFenceIndex;
	std::atomic<uint64_t> copyFenceValue;
	void* requester;
	void(*resourceUploadedPointer)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources);
	void* uploadBufferCpuAddressOfCurrentPos;
	uint64_t uploadResourceOffset;
	RamToVramUploadRequest* uploadRequest;
	uint16_t currentMipLevel;
	uint16_t currentArrayIndex;

	void subresourceUploaded(BaseExecutor* executor, SharedResources& sharedResources)
	{
		if(resourceUploadedPointer) resourceUploadedPointer(requester, executor, sharedResources);
	}

	void useSubresource(BaseExecutor* executor, SharedResources& sharedResources)
	{
		uploadRequest->useSubresource(executor, sharedResources, *this);
	}
};