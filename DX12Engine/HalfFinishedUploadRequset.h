#pragma once
#include <stdint.h>
#include <atomic>

class HalfFinishedUploadRequest
{
public:
	unsigned long numberOfBytesToFree;
	unsigned int copyFenceIndex;
	std::atomic<uint64_t> copyFenceValue;
	void* requester;
	void(*resourceUploadedPointer)(void* context, void* executor, void* sharedResources);
	void* uploadBufferCpuAddressOfCurrentPos;
	uint64_t uploadResourceOffset;
	RamToVramUploadRequest* uploadRequest;
	uint16_t currentMipLevel;
	uint16_t currentArrayIndex;

	static void nullFunction(void* context, void* executor, void* sharedResources) {}

	template<class ThreadResources, class GlobalResources, void(*func)(void*, ThreadResources&, GlobalResources&)>
	void setSubresourceUploadedCallback()
	{
		resourceUploadedPointer = [](void* context, void* executor, void* sharedResources)
		{
			func(context, (ThreadResources&)*executor, (GlobalResources&)*sharedResources);
		}
	}

	void setSubresourceUploadedCallback(void(*func)(void* context, void* executor, void* sharedResources))
	{
		resourceUploadedPointer = func;
	}

	void subresourceUploaded(void* executor, void* sharedResources)
	{
		if(resourceUploadedPointer != &nullFunction) resourceUploadedPointer(requester, executor, sharedResources);
	}

	void useSubresource(void* executor, void* sharedResources)
	{
		uploadRequest->useSubresource(executor, sharedResources, *this);
	}
};