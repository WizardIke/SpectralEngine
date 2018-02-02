#pragma once
#include "D3D12Heap.h"
#include <vector>
#include "TextureResitency.h"
#include "PageCache.h"
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
	std::vector<Chunk> chunks;
	std::vector<Chunk> pinnedChunks;
	size_t mPinnedPageCount = 0u;

	static void allocateChunk(std::vector<Chunk>& chunks, ID3D12Device* graphicsDevice);
	static void findOrMakeFirstFreeChunk(std::vector<Chunk>& chunks, decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice);

	void allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo,
		unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, PageAllocationInfo* pageAllocationInfos, UINT* heapOffsets, UINT* heapTileCounts);

	void allocatePinnedPage(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, VirtualTextureInfo& textureInfo,
		unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, UINT* heapOffsets, UINT* heapTileCounts);

	void allocatePagePacked(decltype(pinnedChunks.begin())& currentChunk, decltype(pinnedChunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue,
		const VirtualTextureInfo& textureInfo, unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILE_REGION_SIZE& tileRegion,
		const D3D12_TILED_RESOURCE_COORDINATE& location, HeapLocation* heapLocations, UINT* heapOffsets, UINT* heapTileCounts);
public:
	void addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice, PageAllocationInfo* pageAllocationInfos);
	void addPackedPages(const VirtualTextureInfo& textureInfo, unsigned int numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice);
	void addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, VirtualTextureInfo& textureInfo, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice);
	void removePages(const PageAllocationInfo* locations, size_t pageCount);
	void removePinnedPages(const HeapLocation* pinnedHeapLocations, unsigned int numPinnedPages);
	size_t pinnedPageCount() { return mPinnedPageCount; }

	template<class TexturesByID>
	void decreaseNonPinnedSize(size_t newSize, PageCache& pageCache, ID3D12CommandQueue* commandQueue,
		const TexturesByID& texturesByID)
	{
		size_t newNumChunks = (newSize + heapSizeInPages - 1u) / heapSizeInPages;
		if (chunks.size() <= newNumChunks) return;
		const size_t heapIdsStart = newNumChunks;
		const size_t heapIdEnd = chunks.size() - 1u;
		std::unique_ptr<textureLocation[]> locations(new textureLocation[(heapIdEnd - heapIdsStart) * heapSizeInPages]);
		textureLocation* locationsEnd = locations.get();
		auto pages = pageCache.pages();


		D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[64u];
		size_t lastIndex = 0u;
		D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
		ID3D12Resource* lastResource;
		const auto end = pages.end();
		auto pagePtr = pages.begin();
		size_t i = 0u;;
		if(pagePtr != end)
		{
			auto textureId = pages.begin()->textureLocation.textureId1();
			const VirtualTextureInfo& textureInfo = texturesByID[textureId];
			lastResource = textureInfo.resource;

			do
			{
				auto& page = *pagePtr;
				if (page.heapLocations.heapIndex >= heapIdsStart && page.heapLocations.heapIndex <= heapIdEnd)
				{
					*locationsEnd = page.textureLocation;
					++locationsEnd;

					auto textureId = page.textureLocation.textureId1();
					const VirtualTextureInfo& textureInfo = texturesByID[textureId];
					resourceTileCoords[i].X = (UINT)page.textureLocation.x();
					resourceTileCoords[i].Y = (UINT)page.textureLocation.y();
					resourceTileCoords[i].Z = 0u;
					resourceTileCoords[i].Subresource = (UINT)page.textureLocation.mipLevel();
					if (lastResource != textureInfo.resource || i == 63u)
					{
						commandQueue->UpdateTileMappings(lastResource, (UINT)(i - lastIndex), resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
							nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
						lastIndex = i;
						lastResource = textureInfo.resource;
					}
					++i;
				}
				++pagePtr;
			} while (pagePtr != end);
			 
			if (i != lastIndex)
			{
				commandQueue->UpdateTileMappings(lastResource, (UINT)(i - lastIndex), resourceTileCoords + lastIndex, nullptr, nullptr, 1u, &rangeFlags, nullptr,
					nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
			}

			for (auto i = locations.get(); i != locationsEnd; ++i)
			{
				pageCache.removePageWithoutDeleting(*i);
			}
		}
	}
};