#pragma once
#include "D3D12Heap.h"
#include "D3D12Resource.h"
#include "ResizingArray.h"
#include "VirtualTextureInfo.h"
#include "VirtualTextureInfoByID.h"
#include "GpuHeapLocation.h"
#include <cstdint> //std::size_t
class GraphicsEngine;
#undef min
#undef max

/*
Allocates and deallocates 64KB pages of gpu memory.
*/
class PageAllocator
{
	constexpr static unsigned long pageSizeInBytes = 64ul * 1024ul;
public:
	constexpr static unsigned long heapSizeInBytes = 32ul * 1024ul * 1024ul;
	constexpr static unsigned short heapSizeInPages = heapSizeInBytes / pageSizeInBytes;
private:
	struct PageInfo
	{
		unsigned short nextIndex;
		unsigned char textureId;
		unsigned char mipLevel;
		unsigned short x;
		unsigned short y;
	};

	struct ResourceDeleteInfo;

	struct IndexedList
	{
		unsigned long nextFreeIndex;
	};

	struct Chunk : public IndexedList
	{
		D3D12Heap data;
		unsigned short freeListIndex;
		PageInfo pageInfos[heapSizeInPages];

		Chunk(ID3D12Device* graphicsDevice, D3D12_HEAP_DESC& heapDesc) :
			data(graphicsDevice, heapDesc)
		{
			freeListIndex = 0u;
			for(unsigned short i = 0u; i != heapSizeInPages; ++i)
			{
				pageInfos[i].nextIndex = i + unsigned short{ 1u };
			}
		}
	};

	struct PackedPageInfo
	{
		unsigned short nextIndex;
	};

	struct PackedChunk : public IndexedList
	{
		D3D12Heap data;
		unsigned short freeListIndex;
		PackedPageInfo pageInfos[heapSizeInPages];

		PackedChunk(ID3D12Device* graphicsDevice, D3D12_HEAP_DESC& heapDesc) :
			data(graphicsDevice, heapDesc)
		{
			freeListIndex = 0u;
			for (unsigned short i = 0u; i != heapSizeInPages; ++i)
			{
				pageInfos[i].nextIndex = i + unsigned short{ 1u };
			}
		}
	};

	IndexedList freeChunks{ std::numeric_limits<unsigned long>::max() };
	ResizingArray<Chunk> allocatedChunks;
	std::size_t pinnedPageCount = 0u;

	IndexedList freePackedChunks{ std::numeric_limits<unsigned long>::max() };
	ResizingArray<PackedChunk> allocatedPackedChunks;

	static void allocatePageHelper(IndexedList& freeChunks, ResizingArray<Chunk>& allocatedChunks, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue,
		ID3D12Resource* resource, unsigned char resourceId, std::size_t& lastIndex, std::size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, GpuHeapLocation& heapLocation,
		UINT* heapOffsets, UINT* heapTileCounts);
public:
	template<class HeapLocationsIterator>
	void addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, std::size_t pageCount, ID3D12Resource* resource, unsigned char resourceId, ID3D12CommandQueue* commandQueue,
		ID3D12Device* graphicsDevice, HeapLocationsIterator heapLocations)
	{
		UINT heapOffsets[heapSizeInPages];
		UINT heapTileCounts[heapSizeInPages];

		std::size_t lastIndex = 0u;
		for (std::size_t i = 0u; i != pageCount; ++i)
		{
			heapTileCounts[i] = 1u;
			allocatePageHelper(freeChunks, allocatedChunks, graphicsDevice, commandQueue, resource,
				resourceId, lastIndex, i, locations, heapLocations[i], heapOffsets, heapTileCounts);
		}

		auto streakLength = static_cast<UINT>(pageCount - lastIndex);
		if (streakLength != 0u)
		{
			auto currentChunkIndex = freeChunks.nextFreeIndex;
			commandQueue->UpdateTileMappings(resource, streakLength, locations + lastIndex, nullptr, allocatedChunks[currentChunkIndex].data, streakLength,
				nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
		}
	}

	void addPackedPages(ID3D12Resource* resource, GpuHeapLocation* pinnedHeapLocations, unsigned int lowestPinnedMip, std::size_t numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice);
	void addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, std::size_t pageCount, ID3D12Resource* resource, unsigned char resourceId,
		GpuHeapLocation* pinnedHeapLocations, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice);
	void removePages(const GpuHeapLocation* heapLocations, std::size_t numPages);
	void removePage(const GpuHeapLocation heapLocation);
	void removePinnedPages(const GpuHeapLocation* pinnedHeapLocations, std::size_t numPinnedPages);
	void removePackedPages(const GpuHeapLocation* pinnedHeapLocations, std::size_t numPinnedPages);
	void decreaseNonPinnedCapacity(std::size_t newSize, VirtualTextureInfoByID& texturesById, GraphicsEngine& graphicsEngine, ID3D12GraphicsCommandList& commandList);
	std::size_t nonPinnedMemoryUsageInPages();
};