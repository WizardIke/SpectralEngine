#pragma once
#include <d3d12.h>
#include "File.h"
#include "PageAllocationInfo.h"

struct VirtualTextureInfo
{
	ID3D12Resource* resource;
	unsigned int widthInPages;
	unsigned int heightInPages;
	unsigned int numMipLevels;
	unsigned int lowestPinnedMip;
	DXGI_FORMAT format;
	unsigned int width;
	unsigned int height;
	File file;
	const wchar_t* filename;
	HeapLocation* pinnedHeapLocations;
};