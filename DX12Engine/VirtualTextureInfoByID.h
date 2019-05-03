#pragma once
#include <type_traits>
#include "VirtualTextureInfo.h"
#include <cassert>

class VirtualTextureInfoByID
{
	union Element
	{
		std::aligned_storage_t<sizeof(VirtualTextureInfo), alignof(VirtualTextureInfo)> data;
		Element* next;
	};
	Element mData[255]; //index 255 is reserved for invalid texture ids
	Element* freeList;
public:
	VirtualTextureInfoByID()
	{
		Element* newFreeList = nullptr;
		for (auto i = 255u; i != 0u;)
		{
			--i;
			mData[i].next = newFreeList;
			newFreeList = &mData[i];
		}
		freeList = newFreeList;
	}

	~VirtualTextureInfoByID()
	{
#ifndef NDEBUG
		unsigned int freeListLength = 0u;
		for (auto i = freeList; i != nullptr; i = i->next)
		{
			++freeListLength;
		}
		assert(freeListLength == 255u && "cannot destruct a TextureInfoAllocator while it is still in use");
#endif
	}

	unsigned int allocate()
	{
		Element* element = freeList;
		freeList = freeList->next;
		return (unsigned int)(element - mData);
	}

	void deallocate(unsigned int index)
	{
		assert(index != 255u);
		mData[index].next = freeList;
		freeList = &mData[index];
	}

	VirtualTextureInfo& operator[](size_t index)
	{
		return reinterpret_cast<VirtualTextureInfo&>(mData[index].data);
	}

	const VirtualTextureInfo& operator[](size_t index) const
	{
		return reinterpret_cast<const VirtualTextureInfo&>(mData[index].data);
	}
};