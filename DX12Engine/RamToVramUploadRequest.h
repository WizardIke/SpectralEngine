#pragma once
#include <d3d12.h>
#include "ScopedFile.h"
class BaseExecutor;

class RamToVramUploadRequest
{
public:
	size_t uploadSizeInBytes;
	uint32_t width, height, depth;
	DXGI_FORMAT format;
	unsigned int currentSubresourceIndex;
	unsigned int numSubresources;
	ScopedFile file;
	void* requester;
	void(*useSubresourcePointer)(RamToVramUploadRequest* const request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset);
	void(*resourceUploadedPointer)(void* const requester, BaseExecutor* const executor);


	void useSubresource(BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
	{
		useSubresourcePointer(this, executor, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset);
	}

	RamToVramUploadRequest() {}

	RamToVramUploadRequest(void(*useSubresourcePointer)(RamToVramUploadRequest* const request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset),
		void(*resourceUploadedPointer)(void* const requester, BaseExecutor* const executor), void* const requester) : useSubresourcePointer(useSubresourcePointer),
		resourceUploadedPointer(resourceUploadedPointer), requester(requester), currentSubresourceIndex(0u) {}
};