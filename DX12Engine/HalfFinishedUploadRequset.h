#pragma once
struct ID3D12Fence;
class BaseExecutor;
#include <stdint.h>

class HalfFinishedUploadRequest
{
public:
	unsigned long numberOfBytesToFree;
	void* requester;

	void(*resourceUploadedPointer)(void* const requester, BaseExecutor* const executor);

	void subresourceUploaded(BaseExecutor* const executor)
	{
		if(resourceUploadedPointer) resourceUploadedPointer(requester, executor);
	}
};