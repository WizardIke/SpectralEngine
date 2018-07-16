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
	constexpr static size_t heapSizeInBytes = 32 * 1024 * 1024;
	constexpr static size_t heapSizeInPages = heapSizeInBytes / (64 * 1024);
private:
	struct Chunk
	{
		D3D12Heap data;
		unsigned int freeList[heapSizeInPages];// can't store free list in heaps because they are on the gpu and we are accessing the free list on the cpu
		unsigned int* freeListEnd;

		Chunk(ID3D12Device* graphicsDevice, D3D12_HEAP_DESC& heapDesc) : data(graphicsDevice, heapDesc)
		{
			freeListEnd = std::end(freeList);
			const auto end = freeListEnd - 1u;
			for(auto i = 0u; i < heapSizeInPages; ++i)
			{
				freeList[i] = i;
			}
		}
	};
	ResizingArray<Chunk> chunks;
	ResizingArray<Chunk> pinnedChunks;
	size_t mPinnedPageCount = 0u;

	static void allocateChunk(ResizingArray<Chunk>& chunks, ID3D12Device* graphicsDevice);
	static void findOrMakeFirstFreeChunk(ResizingArray<Chunk>& chunks, decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice);

	void allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo,
		size_t& lastIndex, size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, PageAllocationInfo* pageAllocationInfos, UINT* heapOffsets, UINT* heapTileCounts);

	void allocatePinnedPage(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo,
		size_t& lastIndex, size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, UINT* heapOffsets, UINT* heapTileCounts);

	void allocatePagePacked(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue,
		const VirtualTextureInfo& textureInfo, size_t& lastIndex, size_t currentIndex, D3D12_TILE_REGION_SIZE& tileRegion,
		const D3D12_TILED_RESOURCE_COORDINATE& location, HeapLocation* heapLocations, UINT* heapOffsets, UINT* heapTileCounts);
public:
	void addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, size_t pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice, PageAllocationInfo* pageAllocationInfos);
	void addPackedPages(const VirtualTextureInfo& textureInfo, size_t numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice);
	void addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, size_t pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice);
	void removePages(const HeapLocation* heapLocations, size_t numPages);
	void removePage(const HeapLocation heapLocation);
	void removePinnedPages(const HeapLocation* pinnedHeapLocations, size_t numPinnedPages);
	size_t pinnedPageCount() { return mPinnedPageCount; }
	void decreaseNonPinnedSize(size_t newSize, PageCache& pageCache, ID3D12CommandQueue* commandQueue, const VirtualTextureInfoByID& texturesByID);
};