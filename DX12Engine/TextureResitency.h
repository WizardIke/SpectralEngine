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
	uint64_t value;

#ifdef false //might be based on endianness
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
		return (value & 0x0000000000ff0000) >> 16u;
	}

	void setMipLevel(uint64_t mipLevel)
	{
		value = (mipLevel << 16u) | (value & 0xffffffffff00ffff);
	}

	void decreaseMipLevel(uint64_t mipLevelDecr)
	{
		value -= mipLevelDecr << 16u;
	}

	uint64_t textureId1() const
	{
		return value & 0x00000000ff000000 >> 24u;
	}

	void setTextureId1(uint64_t idAndSlot)
	{
		value = (idAndSlot << 24u) | (value & 0xffffffff00ffffff);
	}

	uint64_t textureId2() const
	{
		return (value & 0xff00) >> 8u;
	}

	void setTextureId2(uint64_t idAndSlot)
	{
		value = (idAndSlot << 8u) | (value & 0xffffffffffff00ff);
	}

	uint64_t textureId3() const
	{
		return (value & 0xff);
	}

	void setTextureId3(uint64_t idAndSlot)
	{
		value = (idAndSlot ) | (value & 0xffffffffffffff00);
	}
#else
	uint64_t x() const
	{
		return value & 0xffff;
	}

	void setX(uint64_t x)
	{
		value = (x) | (value & 0xffffffffffff0000);
	}

	uint64_t y() const
	{
		return (value & 0x00000000ffff0000) >> 16u;
	}

	void setY(uint64_t y)
	{
		value = (y << 16u) | (value & 0xffffffff0000ffff);
	}

	uint64_t mipLevel() const
	{
		return (value & 0x000000ff00000000) >> 32u;
	}

	void setMipLevel(uint64_t mipLevel)
	{
		value = (mipLevel << 32u) | (value & 0xffffff00ffffffff);
	}

	void decreaseMipLevel(uint64_t mipLevelDecr)
	{
		value -= mipLevelDecr << 32u;
	}

	uint64_t textureId1() const
	{
		return (value & 0x000ff0000000000) >> 40u;
	}

	void setTextureId1(uint64_t idAndSlot)
	{
		value = (idAndSlot << 40u) | (value & 0xffff00ffffffffff);
	}

	uint64_t textureId2() const
	{
		return (value & 0xff00000000000000) >> 56u;
	}

	void setTextureId2(uint64_t idAndSlot)
	{
		value = (idAndSlot << 56u) | (value & 0x00ffffffffffffff);
	}

	uint64_t textureId3() const
	{
		return (value & 0x00ff000000000000) >> 48u;
	}

	void setTextureId3(uint64_t idAndSlot)
	{
		value = (idAndSlot << 48u) | (value & 0x00ffffffffffffff);
	}
#endif

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
	unsigned int pageWidthInTexels;
	unsigned int pageHeightInTexels;
	unsigned int numMipLevels;
	unsigned int lowestPinnedMip;
	DXGI_FORMAT format;
	unsigned int headerSize;
	const wchar_t* fileName;
	HeapLocation* pinnedHeapLocations;
};