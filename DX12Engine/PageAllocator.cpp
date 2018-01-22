#include "PageAllocator.h"
#include <cassert>
#include "TextureResitency.h"

void PageAllocator::allocateChunk(std::vector<Chunk>& chunks, ID3D12Device* graphicsDevice)
{
	D3D12_HEAP_DESC heapDesc;
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDesc.Properties.CreationNodeMask = 1u;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	heapDesc.Properties.VisibleNodeMask = 1u;
	heapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	heapDesc.SizeInBytes = heapSizeInBytes;

	chunks.emplace_back(graphicsDevice, heapDesc);
}

void PageAllocator::findOrMakeFirstFreeChunk(std::vector<Chunk>& chunks, decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice)
{
	while (true)
	{
		if (currentChunk == chunksEnd)
		{
			auto index = currentChunk - chunks.begin();
			allocateChunk(chunks, graphicsDevice);
			currentChunk = chunks.begin() + index;
			chunksEnd = chunks.end();
			break;
		}
		else if (currentChunk->freeList != currentChunk->freeListEnd)
		{
			break;
		}
		++currentChunk;
	}
}

void PageAllocator::allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo,
	unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, PageAllocationInfo* pageAllocationInfos, UINT* heapOffsets)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
			nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(chunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = pageAllocationInfos[currentIndex].heapLocations;
	heapLocation.heapIndex = static_cast<unsigned int>(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::allocatePinnedPage(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice,
	ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo, unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations,
	UINT* heapOffsets)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
			nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = textureInfo.pinnedHeapLocations[currentIndex];
	heapLocation.heapIndex = static_cast<unsigned int>(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::allocatePagePacked(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, const VirtualTextureInfo& textureInfo,
	unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILE_REGION_SIZE& tileRegion, const D3D12_TILED_RESOURCE_COORDINATE& location, HeapLocation* heapLocations, 
	UINT* heapOffsets)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		tileRegion.NumTiles = streakLength;
		commandQueue->UpdateTileMappings(textureInfo.resource, 1u, &location, &tileRegion, currentChunk->data, streakLength,
			nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = heapLocations[0];
	heapLocation.heapIndex = (unsigned int)(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
	ID3D12Device* graphicsDevice, PageAllocationInfo* pageAllocationInfos)
{
	UINT heapOffsets[heapSizeInPages];
	auto currentChunk = chunks.begin();
	auto chunksEnd = chunks.end();
	findOrMakeFirstFreeChunk(chunks, currentChunk, chunksEnd, graphicsDevice);

	unsigned int lastIndex = 0u;
	for (auto i = 0u; i < pageCount; ++i)
	{
		allocatePage(currentChunk, chunksEnd, graphicsDevice, commandQueue, textureInfo, lastIndex, i, locations, pageAllocationInfos, heapOffsets);
	}

	auto streakLength = pageCount - lastIndex;
	commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
		nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::addPackedPages(const VirtualTextureInfo& textureInfo, unsigned int numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice)
{
	assert(numPages <= heapSizeInPages && "All packed pages of a resource must go in the same heap");
	UINT heapOffsets[heapSizeInPages];
	mPinnedPageCount += numPages;
	auto currentChunk = pinnedChunks.begin();
	auto chunksEnd = pinnedChunks.end();
	findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	while (currentChunk->freeListEnd - currentChunk->freeList < numPages)
	{
		++currentChunk;
		findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	}

	D3D12_TILED_RESOURCE_COORDINATE location;
	location.X = 0u;
	location.Y = 0u;
	location.Z = 0u;
	location.Subresource = textureInfo.lowestPinnedMip;

	D3D12_TILE_REGION_SIZE tileRegion;
	tileRegion.NumTiles = numPages;
	tileRegion.UseBox = FALSE;

	auto heapLocations = textureInfo.pinnedHeapLocations;
	
	for (auto i = 0u; i < numPages; ++i)
	{
		--(currentChunk->freeListEnd);
		heapOffsets[i] = *currentChunk->freeListEnd;

		auto& heapLocation = heapLocations[i];
		heapLocation.heapIndex = (unsigned int)(currentChunk - pinnedChunks.begin());
		heapLocation.heapOffsetInPages = heapOffsets[i];
	}
	commandQueue->UpdateTileMappings(textureInfo.resource, 1u, &location, &tileRegion, currentChunk->data, numPages,
		nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
	ID3D12Device* graphicsDevice)
{
	UINT heapOffsets[heapSizeInPages];
	mPinnedPageCount += pageCount;
	auto currentChunk = pinnedChunks.begin();
	auto chunksEnd = pinnedChunks.end();
	findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);

	unsigned int lastIndex = 0u;
	for (auto i = 0u; i < pageCount; ++i)
	{
		allocatePinnedPage(currentChunk, chunksEnd, graphicsDevice, commandQueue, textureInfo, lastIndex, i, locations, heapOffsets);
	}

	auto streakLength = pageCount - lastIndex;
	commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
		nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::removePages(const PageAllocationInfo* locations, unsigned int pageCount)
{
	//free unused heap pages
	for (auto i = 0u; i < pageCount; ++i)
	{
		const HeapLocation& heapLocation = locations[i].heapLocations;
		*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
		++(chunks[heapLocation.heapIndex].freeListEnd);
	}
}

void PageAllocator::removePinnedPages(const HeapLocation* pinnedHeapLocations, unsigned int numPinnedPages)
{
	//free unused heap pages
	mPinnedPageCount -= numPinnedPages;
	for (auto i = 0u; i != numPinnedPages; ++i)
	{
		auto& heapLocation = pinnedHeapLocations[i];
		*pinnedChunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
		++(pinnedChunks[heapLocation.heapIndex].freeListEnd);
	}
}

void PageAllocator::decreaseNonPinnedSize(size_t newSize, PageCache& pageCache, ID3D12CommandQueue* commandQueue,
	const decltype(VirtualTextureManager::texturesByIDAndSlot)& texturesByIDAndSlot)
{
	size_t newNumChunks = newSize / heapSizeInPages;
	assert(chunks.size() > newNumChunks && "can't decrease size to a larger size");
	const size_t heapIdsStart = newNumChunks - 1u;
	const size_t heapIdEnd = chunks.size() - 1u;
	std::unique_ptr<textureLocation[]> locations(new textureLocation[(heapIdEnd - heapIdsStart) * heapSizeInPages]);
	textureLocation* locationsEnd = locations.get();
	auto pages = pageCache.pages();


	D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[64u];
	size_t lastIndex = 0u;
	D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
	ID3D12Resource* lastResource;
	{
		auto textureId = pages.begin()->textureLocation.textureId();
		auto textureSlot = pages.begin()->textureLocation.textureSlots();
		const VirtualTextureInfo& textureInfo = texturesByIDAndSlot[textureSlot].data()[textureId];
		lastResource = textureInfo.resource;
	}
	size_t i = 0u;
	for (const PageAllocationInfo& page : pages)
	{
		if (page.heapLocations.heapIndex >= heapIdsStart && page.heapLocations.heapIndex <= heapIdEnd)
		{
			*locationsEnd = page.textureLocation;
			++locationsEnd;

			auto textureId = page.textureLocation.textureId();
			auto textureSlot = page.textureLocation.textureSlots();
			const VirtualTextureInfo& textureInfo = texturesByIDAndSlot[textureSlot].data()[textureId];
			resourceTileCoords[i].X = page.textureLocation.x();
			resourceTileCoords[i].Y = page.textureLocation.y();
			resourceTileCoords[i].Z = 0u;
			resourceTileCoords[i].Subresource = page.textureLocation.mipLevel();
			if (lastResource != textureInfo.resource || i == 63u)
			{
				commandQueue->UpdateTileMappings(lastResource, i - lastIndex, resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
					nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
				lastIndex = i;
				lastResource = textureInfo.resource;
			}
			++i;
		}
	}
	commandQueue->UpdateTileMappings(lastResource, i - lastIndex, resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
		nullptr, D3D12_TILE_MAPPING_FLAG_NONE);

	for (auto i = locations.get(); i != locationsEnd; ++i)
	{
		pageCache.removePageWithoutDeleting(*i);
	}
}