#include "PageDeleter.h"

PageDeleter::PageDeleter(PageAllocator& pageAllocator, const decltype(VirtualTextureManager::texturesByIDAndSlot)& texturesByIDAndSlot, ID3D12CommandQueue* commandQueue) :
	pageAllocator(pageAllocator),
	texturesByIDAndSlot(texturesByIDAndSlot),
	commandQueue(commandQueue) {}

void PageDeleter::operator()(PageAllocationInfo& allocationInfo)
{
	deletedPages[deletedPageCount] = allocationInfo;
	++deletedPageCount;
	if (deletedPageCount == maxPendingDeletedPages)
	{
		finish();
		deletedPageCount = 0u;
	}
}

void PageDeleter::finish()
{
	if (deletedPageCount == 0u) return;
	pageAllocator.removePages(deletedPages, deletedPageCount);

	D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[maxPendingDeletedPages];
	size_t lastIndex = 0u;
	D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
	ID3D12Resource* lastResource;
	{
		auto textureId = deletedPages[0].textureLocation.textureId();
		auto textureSlot = deletedPages[0].textureLocation.textureSlots();
		const VirtualTextureInfo& textureInfo = texturesByIDAndSlot[textureSlot].data()[textureId];
		lastResource = textureInfo.resource;
	}
	for (size_t i = 0u; i != deletedPageCount; ++i)
	{
		auto textureId = deletedPages[i].textureLocation.textureId();
		auto textureSlot = deletedPages[i].textureLocation.textureSlots();
		const VirtualTextureInfo& textureInfo = texturesByIDAndSlot[textureSlot].data()[textureId];
		resourceTileCoords[i].X = (UINT)deletedPages[i].textureLocation.x();
		resourceTileCoords[i].Y = (UINT)deletedPages[i].textureLocation.y();
		resourceTileCoords[i].Z = 0u;
		resourceTileCoords[i].Subresource = (UINT)deletedPages[i].textureLocation.mipLevel();
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
}