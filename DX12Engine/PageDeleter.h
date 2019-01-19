#pragma once
#include "PageAllocator.h"
#include "PageAllocationInfo.h"
#include "VirtualTextureInfoByID.h"
class PageAllocator;

class PageDeleter
{
	constexpr static unsigned int maxPendingDeletedPages = 32u;
	PageAllocator& pageAllocator;
	TextureLocation deletedPages[maxPendingDeletedPages];
	size_t deletedPageCount = 0u;
	const VirtualTextureInfoByID& texturesByID;
	ID3D12CommandQueue* commandQueue;

	void finishNoCheck();
public:
	PageDeleter(PageAllocator& pageAllocator, const VirtualTextureInfoByID& texturesByID, ID3D12CommandQueue* commandQueue);
	void deletePage(const PageAllocationInfo& allocationInfo);
	/* Delete page from resource without freeing heap space */
	void deletePage(const TextureLocation& textureLocation);

	void finish()
	{
		if (deletedPageCount != 0u)
		{
			finishNoCheck();
		}
	}
};