#pragma once
#include <d3d12.h>
#include "ScopedFile.h"
#include <cstdint>
class BaseExecutor;
class SharedResources;
class HalfFinishedUploadRequest;

struct MeshHeader
{
	uint32_t verticesSize;
	uint32_t indicesSize;
	uint32_t sizeInBytes;
	uint16_t padding;
};

struct TextureHeader
{
	uint32_t width;
	DXGI_FORMAT format;
	uint32_t height;
	uint16_t depth;
};

class RamToVramUploadRequest
{
public:
	File file;
	void* requester;
	ID3D12Resource* uploadResource;
	void(*useSubresource)(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest);
	void(*resourceUploadedPointer)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources);

	D3D12_RESOURCE_DIMENSION dimension;
	union
	{
		MeshHeader meshInfo;
		TextureHeader textureInfo;
	};
	uint16_t mipLevels;
	uint16_t mostDetailedMip;
	uint16_t arraySize;
	uint16_t currentMipLevel; //Should not be read by resource loading threads. Instead use the copy in HalfFinishedUploadRequest
	uint16_t currentArrayIndex; //Should not be read by resource loading threads. Instead use the copy in HalfFinishedUploadRequest
};