#pragma once
#include <d3d12.h>
#include "ScopedFile.h"
#include <cstdint>
class BaseExecutor;
class SharedResources;

class RamToVramUploadRequest
{
public:
	ScopedFile file;
	void* requester;
	void(*useSubresourcePointer)(RamToVramUploadRequest* const request, BaseExecutor* executor, SharedResources& sharedResources, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset);
	void(*resourceUploadedPointer)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources);

	DXGI_FORMAT format;
	D3D12_RESOURCE_DIMENSION dimension;
	uint64_t width;
	uint32_t height;
	uint16_t depth;
	uint16_t arraySize;
	uint16_t mipLevels;
	uint16_t currentArrayIndex;
	uint16_t currentMipLevel;
	uint16_t mostDetailedMip;

	void useSubresource(BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, SharedResources& sharedResources, uint64_t uploadResourceOffset)
	{
		useSubresourcePointer(this, executor, sharedResources, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset);
	}
};