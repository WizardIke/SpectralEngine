#pragma once
#include <d3d12.h>

struct HeapLocation
{
	unsigned int heapIndex;
	unsigned int heapOffsetInPages;

	HeapLocation() {} //so std::vector doesn't zero heapIndex and heapOffsetInPages when it doesn't need to
	HeapLocation(unsigned int heapIndex, unsigned int heapOffsetInPages) : heapIndex(heapIndex), heapOffsetInPages(heapOffsetInPages) {} //needed because of other constructor
};

struct PageRequestData
{
	unsigned int count = 0u;
};

// pages are stored as x, y, miplevel, textureIdAndSlot
class textureLocation
{
public:
	constexpr static unsigned int maxTextureSlots = 6;
	uint64_t value;

	uint64_t x() const
	{
		return value >> 48u;
	}

	void setX(uint64_t x)
	{
		value = (x << 48u) | (value & 0x0000ffffffffffff);
	}

	uint64_t y() const
	{
		return (value & 0x0000ffff00000000) >> 32u;
	}

	void setY(uint64_t y)
	{
		value = (y << 32u) | (value & 0xffff0000ffffffff);
	}

	uint64_t mipLevel() const
	{
		return (value & 0x00000000ffff0000) >> 16u;
	}

	void setMipLevel(uint64_t mipLevel)
	{
		value = (mipLevel << 16u) | (value & 0xffffffff0000ffff);
	}

	void decreaseMipLevel(uint64_t mipLevelDecr)
	{
		value += mipLevelDecr << 16u;
	}

	uint64_t textureIdAndSlot() const
	{
		return value & 0x000000000000ffff;
	}

	void setTextureIdAndSlot(uint64_t idAndSlot)
	{
		value = idAndSlot | (value & 0xffffffffffff0000);
	}

	uint64_t textureId() const
	{
		return value & 0x03FF;
	}

	uint64_t textureSlots() const
	{
		return (value & 0xFC00) >> 10u;
	}

	void setTextureSlots(uint64_t textureSlots)
	{
		value = (textureSlots << 10u) | (value & 0xffffffffffff03ff);
	}

	bool operator==(const textureLocation& other) const
	{
		return value == other.value;
	}
};

struct PageAllocationInfo
{
	HeapLocation heapLocations;
	textureLocation textureLocation; //value == std::numeric_limits<uint64_t>::max() when the page is invalid
};

struct VirtualTextureInfo
{
	ID3D12Resource* resource;
	unsigned int widthInPages;
	unsigned int heightInPages;
	unsigned int widthInTexels;
	unsigned int heightInTexels;
	unsigned int numMipLevels;
	unsigned int lowestPinnedMip;
	DXGI_FORMAT format;
	unsigned int headerSize;
	const wchar_t* fileName;
	HeapLocation* pinnedHeapLocations;
};