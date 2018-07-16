#include "PageAllocator.h"
#include <cassert>
#include "PageDeleter.h"

void PageAllocator::allocateChunk(ResizingArray<Chunk>& chunks, ID3D12Device* graphicsDevice)
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

void PageAllocator::findOrMakeFirstFreeChunk(ResizingArray<Chunk>& chunks, decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice)
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
	size_t& lastIndex, size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, PageAllocationInfo* pageAllocationInfos, UINT* heapOffsets, UINT* heapTileCounts)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(chunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = pageAllocationInfos[currentIndex].heapLocation;
	heapLocation.heapIndex = static_cast<unsigned int>(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::allocatePinnedPage(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice,
	ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo, size_t& lastIndex, size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations,
	UINT* heapOffsets, UINT* heapTileCounts)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = textureInfo.pinnedHeapLocations[currentIndex];
	heapLocation.heapIndex = static_cast<unsigned int>(currentChunk - pinnedChunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::allocatePagePacked(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, const VirtualTextureInfo& textureInfo,
	size_t& lastIndex, size_t currentIndex, D3D12_TILE_REGION_SIZE& tileRegion, const D3D12_TILED_RESOURCE_COORDINATE& location, HeapLocation* heapLocations,
	UINT* heapOffsets, UINT* heapTileCounts)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		tileRegion.NumTiles = (UINT)streakLength;
		commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)1u, &location, &tileRegion, currentChunk->data, (UINT)streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = heapLocations[0];
	heapLocation.heapIndex = (unsigned int)(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, size_t pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
	ID3D12Device* graphicsDevice, PageAllocationInfo* pageAllocationInfos)
{
	UINT heapOffsets[heapSizeInPages];
	UINT heapTileCounts[heapSizeInPages];
	auto currentChunk = chunks.begin();
	auto chunksEnd = chunks.end();
	findOrMakeFirstFreeChunk(chunks, currentChunk, chunksEnd, graphicsDevice);

	size_t lastIndex = 0u;
	for (size_t i = 0u; i != pageCount; ++i)
	{
		heapTileCounts[i] = 1u;
		allocatePage(currentChunk, chunksEnd, graphicsDevice, commandQueue, textureInfo, lastIndex, i, locations, pageAllocationInfos, heapOffsets, heapTileCounts);
	}

	auto streakLength = pageCount - lastIndex;
	commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
		nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::addPackedPages(const VirtualTextureInfo& textureInfo, size_t numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice)
{
	assert(numPages <= heapSizeInPages && "All packed pages of a resource must go in the same heap");
	UINT heapOffsets[heapSizeInPages];
	UINT heapTileCounts[heapSizeInPages];
	mPinnedPageCount += numPages;
	auto currentChunk = pinnedChunks.begin();
	auto chunksEnd = pinnedChunks.end();
	findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	while ((size_t)(currentChunk->freeListEnd - currentChunk->freeList) < numPages)
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
	tileRegion.NumTiles = (UINT)numPages;
	tileRegion.UseBox = FALSE;

	auto heapLocations = textureInfo.pinnedHeapLocations;
	
	for (auto i = 0u; i < numPages; ++i)
	{
		heapTileCounts[i] = 1u;
		--(currentChunk->freeListEnd);
		heapOffsets[i] = *currentChunk->freeListEnd;

		auto& heapLocation = heapLocations[i];
		heapLocation.heapIndex = (unsigned int)(currentChunk - pinnedChunks.begin());
		heapLocation.heapOffsetInPages = heapOffsets[i];
	}
	commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)1u, &location, &tileRegion, currentChunk->data, (UINT)numPages,
		nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, size_t pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
	ID3D12Device* graphicsDevice)
{
	UINT heapOffsets[heapSizeInPages];
	UINT heapTileCounts[heapSizeInPages];
	mPinnedPageCount += pageCount;
	auto currentChunk = pinnedChunks.begin();
	auto chunksEnd = pinnedChunks.end();
	findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);

	size_t lastIndex = 0u;
	for (size_t i = 0u; i != pageCount; ++i)
	{
		heapTileCounts[i] = 1u;
		allocatePinnedPage(currentChunk, chunksEnd, graphicsDevice, commandQueue, textureInfo, lastIndex, i, locations, heapOffsets, heapTileCounts);
	}

	size_t streakLength = pageCount - lastIndex;
	commandQueue->UpdateTileMappings(textureInfo.resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
		nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::removePages(const HeapLocation* heapLocations, const size_t pageCount)
{
	//free unused heap pages
	for (size_t i = 0u; i != pageCount; ++i)
	{
		const HeapLocation& heapLocation = heapLocations[i];
		*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
		++(chunks[heapLocation.heapIndex].freeListEnd);
	}
}

void PageAllocator::removePage(const HeapLocation heapLocation)
{
	*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
	++(chunks[heapLocation.heapIndex].freeListEnd);
}

void PageAllocator::removePinnedPages(const HeapLocation* pinnedHeapLocations, const size_t numPinnedPages)
{
	//free unused heap pages
	mPinnedPageCount -= numPinnedPages;
	for (size_t i = 0u; i != numPinnedPages; ++i)
	{
		auto& heapLocation = pinnedHeapLocations[i];
		*pinnedChunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
		++(pinnedChunks[heapLocation.heapIndex].freeListEnd);
	}
}

void PageAllocator::decreaseNonPinnedSize(size_t newSize, PageCache& pageCache, ID3D12CommandQueue* commandQueue, const VirtualTextureInfoByID& texturesByID)
{
	size_t newNumChunks = (newSize + heapSizeInPages - 1u) / heapSizeInPages;
	if (chunks.size() <= newNumChunks) return;
	const size_t heapIdsStart = newNumChunks;
	const size_t heapIdEnd = chunks.size() - 1u;

	PageDeleter pageDeleter(*this, texturesByID, commandQueue);

	auto pages = pageCache.pages();
	const auto end = pages.end();
	;
	for (auto pagePtr = pages.begin(); pagePtr != end; ++pagePtr)
	{
		auto& page = *pagePtr;
		if (page.heapLocation.heapIndex >= heapIdsStart && page.heapLocation.heapIndex <= heapIdEnd)
		{
			pageCache.removePageWithoutDeleting(page.textureLocation); //remove page from cache
			pageDeleter.deletePage(page.textureLocation); //remove page from resource
		}
	}
	pageDeleter.finish();
}