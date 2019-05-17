#include "PageDeleter.h"
#include "PageAllocator.h"
#include <algorithm> //std::sort

void PageDeleter::finishNoCheck(const VirtualTextureInfoByID& texturesByID)
{
	//group pages from the same resource together. 
	std::sort(deletedPages, deletedPages + deletedPageCount, [](PageResourceLocation first, PageResourceLocation second)
	{
		return first.textureId < second.textureId;
	});

	D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[maxPendingDeletedPages];
	std::size_t lastIndex = 0u;
	const D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
	unsigned short lastTextureId = deletedPages[0].textureId;
	for (std::size_t i = 0u; i != deletedPageCount; ++i)
	{
		resourceTileCoords[i].X = (UINT)deletedPages[i].x;
		resourceTileCoords[i].Y = (UINT)deletedPages[i].y;
		resourceTileCoords[i].Z = 0u;
		resourceTileCoords[i].Subresource = (UINT)deletedPages[i].mipLevel;

		auto textureId = deletedPages[i].textureId;
		if (lastTextureId != textureId)
		{
			const VirtualTextureInfo& textureInfo = texturesByID[textureId];
			commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)(i - lastIndex), resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
				nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
			lastIndex = i;
			lastTextureId = textureId;
		}
	}
	const VirtualTextureInfo& textureInfo = texturesByID[lastTextureId];
	commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)(deletedPageCount - lastIndex), resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
		nullptr, D3D12_TILE_MAPPING_FLAG_NONE);

	deletedPageCount = 0u;
}

PageDeleter::PageDeleter(PageAllocator& pageAllocator1, ID3D12CommandQueue* commandQueue1) :
	pageAllocator(pageAllocator1),
	commandQueue(commandQueue1) {}

void PageDeleter::deletePage(PageAllocationInfo allocationInfo, const VirtualTextureInfoByID& texturesByID)
{
	pageAllocator.removePage(allocationInfo.heapLocation);
	deletePage(allocationInfo.textureLocation, texturesByID);
}

void PageDeleter::deletePage(PageResourceLocation textureLocation, const VirtualTextureInfoByID& texturesByID)
{
	deletedPages[deletedPageCount] = textureLocation;
	++deletedPageCount;
	if (deletedPageCount == maxPendingDeletedPages)
	{
		finishNoCheck(texturesByID);
	}
}