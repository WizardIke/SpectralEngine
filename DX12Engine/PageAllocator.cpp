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

void PageAllocator::allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, ID3D12Resource* resource,
	size_t& lastIndex, size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, GpuHeapLocation& heapLocation, UINT* heapOffsets, UINT* heapTileCounts)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(chunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	heapLocation.heapIndex = static_cast<unsigned int>(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::allocatePinnedPage(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice,
	ID3D12CommandQueue* commandQueue, ID3D12Resource* resource, GpuHeapLocation* pinnedHeapLocations, size_t& lastIndex, size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations,
	UINT* heapOffsets, UINT* heapTileCounts)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(pinnedChunks, currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = pinnedHeapLocations[currentIndex];
	heapLocation.heapIndex = static_cast<unsigned int>(currentChunk - pinnedChunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageAllocator::addPinnedPages(ID3D12Resource* resource, GpuHeapLocation* pinnedHeapLocations, unsigned int lowestPinnedMip, std::size_t numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice)
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
	location.Subresource = lowestPinnedMip;

	D3D12_TILE_REGION_SIZE tileRegion;
	tileRegion.NumTiles = (UINT)numPages;
	tileRegion.UseBox = FALSE;
	
	for (auto i = 0u; i < numPages; ++i)
	{
		heapTileCounts[i] = 1u;
		--(currentChunk->freeListEnd);
		heapOffsets[i] = *currentChunk->freeListEnd;

		auto& heapLocation = pinnedHeapLocations[i];
		heapLocation.heapIndex = (unsigned int)(currentChunk - pinnedChunks.begin());
		heapLocation.heapOffsetInPages = heapOffsets[i];
	}
	commandQueue->UpdateTileMappings(resource, (UINT)1u, &location, &tileRegion, currentChunk->data, (UINT)numPages,
		nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, size_t pageCount, ID3D12Resource* resource, GpuHeapLocation* pinnedHeapLocations, ID3D12CommandQueue* commandQueue,
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
		allocatePinnedPage(currentChunk, chunksEnd, graphicsDevice, commandQueue, resource, pinnedHeapLocations, lastIndex, i, locations, heapOffsets, heapTileCounts);
	}

	size_t streakLength = pageCount - lastIndex;
	commandQueue->UpdateTileMappings(resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
		nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::removePages(const GpuHeapLocation* heapLocations, const size_t pageCount)
{
	//free unused heap pages
	for (size_t i = 0u; i != pageCount; ++i)
	{
		const GpuHeapLocation& heapLocation = heapLocations[i];
		*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
		++(chunks[heapLocation.heapIndex].freeListEnd);
	}
}

void PageAllocator::removePage(const GpuHeapLocation heapLocation)
{
	*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
	++(chunks[heapLocation.heapIndex].freeListEnd);
}

void PageAllocator::removePinnedPages(const GpuHeapLocation* pinnedHeapLocations, const size_t numPinnedPages)
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

void PageAllocator::decreaseNonPinnedSize(size_t newSize, PageCache& pageCache, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter)
{
	size_t newNumChunks = (newSize + heapSizeInPages - 1u) / heapSizeInPages;
	if (chunks.size() <= newNumChunks) return;
	const size_t heapIdsStart = newNumChunks;
	const size_t heapIdsEnd = chunks.size() - 1u;

	auto pages = pageCache.pages();
	const auto end = pages.end();
	;
	for (auto pagePtr = pages.begin(); pagePtr != end; ++pagePtr)
	{
		auto& page = *pagePtr;
		if (page.heapLocation.heapIndex >= heapIdsStart && page.heapLocation.heapIndex <= heapIdsEnd)
		{
			VirtualTextureInfo& textureInfo = texturesById[page.textureLocation.textureId];
			pageCache.removePage(page.textureLocation, textureInfo, texturesById, pageDeleter); //remove page from cache
		}
	}
}

std::size_t PageAllocator::nonPinnedMemoryUsageInPages()
{
	return chunks.size() * heapSizeInPages;
}