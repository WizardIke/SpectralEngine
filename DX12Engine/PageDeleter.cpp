#include "PageDeleter.h"
#include "PageAllocator.h"

void PageDeleter::finishNoCheck()
{
	D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[maxPendingDeletedPages];
	size_t lastIndex = 0u;
	D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
	ID3D12Resource* lastResource;
	{
		auto textureId = deletedPages[0].textureId1();
		const VirtualTextureInfo& textureInfo = texturesByID[textureId];
		lastResource = textureInfo.resource;
	}
	for (size_t i = 0u; i != deletedPageCount; ++i)
	{
		auto textureId = deletedPages[i].textureId1();
		const VirtualTextureInfo& textureInfo = texturesByID[textureId];
		resourceTileCoords[i].X = (UINT)deletedPages[i].x();
		resourceTileCoords[i].Y = (UINT)deletedPages[i].y();
		resourceTileCoords[i].Z = 0u;
		resourceTileCoords[i].Subresource = (UINT)deletedPages[i].mipLevel();
		if (lastResource != textureInfo.resource)
		{
			commandQueue->UpdateTileMappings(lastResource, (UINT)(i - lastIndex), resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
				nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
			lastIndex = i;
			lastResource = textureInfo.resource;
		}
	}
	commandQueue->UpdateTileMappings(lastResource, (UINT)(deletedPageCount - lastIndex), resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
		nullptr, D3D12_TILE_MAPPING_FLAG_NONE);

	deletedPageCount = 0u;
}

PageDeleter::PageDeleter(PageAllocator& pageAllocator, const VirtualTextureInfoByID& texturesByID, ID3D12CommandQueue* commandQueue) :
	pageAllocator(pageAllocator),
	texturesByID(texturesByID),
	commandQueue(commandQueue) {}

void PageDeleter::deletePage(const PageAllocationInfo& allocationInfo)
{
	pageAllocator.removePage(allocationInfo.heapLocation);
	deletePage(allocationInfo.textureLocation);
}

void PageDeleter::deletePage(const TextureLocation& textureLocation)
{
	deletedPages[deletedPageCount] = textureLocation;
	++deletedPageCount;
	if (deletedPageCount == maxPendingDeletedPages)
	{
		finishNoCheck();
	}
}