#pragma once
#include "VirtualTextureInfo.h"
#include <cassert>

class VirtualTextureInfoByID
{
	union Element
	{
		VirtualTextureInfo* data;
		Element* next;

		Element() {}
		~Element() {}
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

	unsigned int allocate(VirtualTextureInfo& info)
	{
		Element* element = freeList;
		freeList = freeList->next;
		element->data = &info;
		return (unsigned int)(element - mData);
	}

	void deallocate(unsigned int index)
	{
		assert(index != 255u);
		mData[index].next = freeList;
		freeList = &mData[index];
	}

	VirtualTextureInfo& operator[](unsigned int index)
	{
		return *mData[index].data;
	}

	const VirtualTextureInfo& operator[](unsigned int index) const
	{
		return *mData[index].data;
	}
};