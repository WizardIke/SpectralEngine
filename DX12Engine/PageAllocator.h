#pragma once
#include "D3D12Heap.h"
#include "ResizingArray.h"
#include "PageAllocationInfo.h"
#include "VirtualTextureInfo.h"
#include "PageCache.h"
#include "VirtualTextureInfoByID.h"
#undef min
#undef max

/*
Allocates and deallocates 64KB pages of gpu memory.
*/
class PageAllocator
{
public:
	constexpr static std::size_t heapSizeInBytes = 32 * 1024 * 1024;
	constexpr static std::size_t heapSizeInPages = heapSizeInBytes / (64 * 1024);
private:
	struct Chunk
	{
		D3D12Heap data;
		unsigned int freeList[heapSizeInPages];// can't store free list in heaps because they are on the gpu and we are accessing the free list on the cpu
		unsigned int* freeListEnd;

		Chunk(ID3D12Device* graphicsDevice, D3D12_HEAP_DESC& heapDesc) : data(graphicsDevice, heapDesc)
		{
			freeListEnd = std::end(freeList);
			for(auto i = 0u; i != heapSizeInPages; ++i)
			{
				freeList[i] = i;
			}
		}
	};
	ResizingArray<Chunk> chunks;
	ResizingArray<Chunk> pinnedChunks;
	std::size_t mPinnedPageCount = 0u;

	static void allocateChunk(ResizingArray<Chunk>& chunks, ID3D12Device* graphicsDevice);
	static void findOrMakeFirstFreeChunk(ResizingArray<Chunk>& chunks, decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice);

	void allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, ID3D12Resource* resource,
		std::size_t& lastIndex, std::size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, HeapLocation& heapLocation, UINT* heapOffsets, UINT* heapTileCounts);

	void allocatePinnedPage(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, ID3D12Resource* resource,
		HeapLocation* pinnedHeapLocations, std::size_t& lastIndex, std::size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, UINT* heapOffsets, UINT* heapTileCounts);
public:
	template<class HeapLocationsIterator>
	void addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, std::size_t pageCount, ID3D12Resource* resource, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice, HeapLocationsIterator heapLocations)
	{
		UINT heapOffsets[heapSizeInPages];
		UINT heapTileCounts[heapSizeInPages];
		auto currentChunk = chunks.begin();
		auto chunksEnd = chunks.end();
		findOrMakeFirstFreeChunk(chunks, currentChunk, chunksEnd, graphicsDevice);

		std::size_t lastIndex = 0u;
		for(std::size_t i = 0u; i != pageCount; ++i)
		{
			heapTileCounts[i] = 1u;
			allocatePage(currentChunk, chunksEnd, graphicsDevice, commandQueue, resource, lastIndex, i, locations, heapLocations[i], heapOffsets, heapTileCounts);
		}

		auto streakLength = pageCount - lastIndex;
		commandQueue->UpdateTileMappings(resource, (UINT)streakLength, locations + lastIndex, nullptr, currentChunk->data, (UINT)streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
	}

	void addPinnedPages(ID3D12Resource* resource, HeapLocation* pinnedHeapLocations, unsigned int lowestPinnedMip, std::size_t numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice);
	void addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, std::size_t pageCount, ID3D12Resource* resource, HeapLocation* pinnedHeapLocations, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice);
	void removePages(const HeapLocation* heapLocations, std::size_t numPages);
	void removePage(const HeapLocation heapLocation);
	void removePinnedPages(const HeapLocation* pinnedHeapLocations, std::size_t numPinnedPages);
	std::size_t pinnedPageCount() { return mPinnedPageCount; }
	void decreaseNonPinnedSize(std::size_t newSize, PageCache& pageCache, PageDeleter& pageDeleter);
	std::size_t nonPinnedMemoryUsageInPages();
};