#pragma once
#include "PageAllocator.h"
#include "PageAllocationInfo.h"
#include "VirtualTextureInfoByID.h"
class PageAllocator;

class PageDeleter
{
	constexpr static unsigned int maxPendingDeletedPages = 32u;
	PageAllocator& pageAllocator;
	PageResourceLocation deletedPages[maxPendingDeletedPages];
	size_t deletedPageCount = 0u;
	ID3D12CommandQueue* commandQueue;

	void finishNoCheck(VirtualTextureInfoByID& texturesByID);
public:
	PageDeleter(PageAllocator& pageAllocator, ID3D12CommandQueue* commandQueue);
	void deletePage(PageAllocationInfo allocationInfo, VirtualTextureInfoByID& texturesByID);
	/* Delete page from resource without freeing heap space */
	void deletePage(PageResourceLocation textureLocation, VirtualTextureInfoByID& texturesByID);

	void finish(VirtualTextureInfoByID& texturesByID)
	{
		if (deletedPageCount != 0u)
		{
			finishNoCheck(texturesByID);
		}
	}
};