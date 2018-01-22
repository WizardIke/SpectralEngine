#pragma once
#include "PageAllocator.h"
#include "TextureResitency.h"
#include "VirtualTextureManager.h"

class PageDeleter
{
	constexpr static unsigned int maxPendingDeletedPages = 32u;
	PageAllocator& pageAllocator;
	PageAllocationInfo deletedPages[maxPendingDeletedPages];
	size_t deletedPageCount = 0u;
	const decltype(VirtualTextureManager::texturesByIDAndSlot)& texturesByIDAndSlot;
	ID3D12CommandQueue* commandQueue;
public:
	PageDeleter(PageAllocator& pageAllocator, const decltype(VirtualTextureManager::texturesByIDAndSlot)& texturesByIDAndSlot, ID3D12CommandQueue* commandQueue);
	void operator()(PageAllocationInfo& allocationInfo);
	void finish();
};