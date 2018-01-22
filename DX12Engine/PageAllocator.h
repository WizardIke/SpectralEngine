#pragma once
#include "D3D12Heap.h"
#include <vector>
#include "TextureResitency.h"
#include "PageCache.h"
#include "VirtualTextureManager.h"

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
	std::vector<Chunk> chunks;
	std::vector<Chunk> pinnedChunks;
	size_t mPinnedPageCount = 0u;

	static void allocateChunk(std::vector<Chunk>& chunks, ID3D12Device* graphicsDevice);
	static void findOrMakeFirstFreeChunk(std::vector<Chunk>& chunks, decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice);

	void allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo,
		unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, PageAllocationInfo* pageAllocationInfos, UINT* heapOffsets);

	void allocatePinnedPage(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo,
		unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, UINT* heapOffsets);

	void allocatePagePacked(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue,
		const VirtualTextureInfo& textureInfo, unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILE_REGION_SIZE& tileRegion,
		const D3D12_TILED_RESOURCE_COORDINATE& location, HeapLocation* heapLocations, UINT* heapOffsets);
public:
	void addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice, PageAllocationInfo* pageAllocationInfos);
	void addPackedPages(const VirtualTextureInfo& textureInfo, unsigned int numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice);
	void addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice);
	void removePages(const PageAllocationInfo* locations, unsigned int pageCount);
	void removePinnedPages(const HeapLocation* pinnedHeapLocations, unsigned int numPinnedPages);
	void decreaseNonPinnedSize(size_t newSize, PageCache& pageCache, ID3D12CommandQueue* commandQueue,
		const decltype(VirtualTextureManager::texturesByIDAndSlot)& texturesByIDAndSlot);
	size_t pinnedPageCount() { return mPinnedPageCount; }
};