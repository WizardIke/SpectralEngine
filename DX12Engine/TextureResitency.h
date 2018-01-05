#pragma once
#include <d3d12.h>

struct HeapLocation
{
	unsigned int heapIndex;
	unsigned int heapOffsetInPages;

	HeapLocation() {} //so std::vector doesn't zero heapIndex and heapOffsetInPages when it doesn't need to
	HeapLocation(unsigned int heapIndex, unsigned int heapOffsetInPages) : heapIndex(heapIndex), heapOffsetInPages(heapOffsetInPages) {} //needed because of other constructor
};

struct TextureResitency
{
	ID3D12Resource* resource;
	unsigned int widthInPages;
	unsigned int heightInPages;
	unsigned int widthInTexels;
	unsigned int heightInTexels;
	unsigned int numMipLevels;
	unsigned int mostDetailedPackedMip;
	unsigned int numTilesForPackedMips;
	unsigned char* mostDetailedMips;
	HeapLocation* heapLocations;
};