#pragma once
#include "D3D12Heap.h"
#include <vector>
#include "TextureResitency.h"

/*
Allocates and deallocates 64KB pages of gpu memory.
*/
class PageProvider
{
	constexpr static size_t heapSizeInBytes = 32 * 1024 * 1024;
	constexpr static size_t heapSizeInPages = heapSizeInBytes / (64 * 1024);
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
	UINT heapOffsets[heapSizeInPages];

	void allocateChunk(ID3D12Device* graphicsDevice);

	void findOrMakeFirstFreeChunk(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice);

	static unsigned long resourceLocationToHeapLocationIndex(const D3D12_TILED_RESOURCE_COORDINATE& resourceLocation, const TextureResitency& textureInfo);

	void allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo,
		unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags);

	void allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo,
		unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations);

	void allocatePagePacked(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, const TextureResitency& textureInfo,
		unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILE_REGION_SIZE& tileRegion, const D3D12_TILED_RESOURCE_COORDINATE& location, HeapLocation* heapLocations);

	void addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags, unsigned int pageCount, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo, ID3D12Device* graphicsDevice);

	void removePages(D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags, unsigned int pageCount, TextureResitency& textureInfo);
public:
	void updatePages(D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags, unsigned int pageCount, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo, ID3D12Device* graphicsDevice);

	void addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, TextureResitency& textureInfo, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice);

	void addPackedPages(const TextureResitency& textureInfo, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice, HeapLocation* heapLocations);

	void removePages(const D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, const TextureResitency& textureInfo);

	void removePackedPages(const D3D12_TILED_RESOURCE_COORDINATE& location, const D3D12_TILE_REGION_SIZE& tileRegion, TextureResitency& textureInfo);
};