#pragma once
struct ID3D12Fence;
class BaseExecutor;
class SharedResources;
#include <stdint.h>

class HalfFinishedUploadRequest
{
public:
	unsigned long numberOfBytesToFree;
	void* requester;

	void(*resourceUploadedPointer)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources);

	void subresourceUploaded(BaseExecutor* const executor, SharedResources& sharedResources)
	{
		if(resourceUploadedPointer) resourceUploadedPointer(requester, executor, sharedResources);
	}
};