#include "PageAllocator.h"
#include <cassert>
#include "PageDeleter.h"
#include <limits>
#include "GraphicsEngine.h"
#include <bitset>
#include "FixedCapacityFastIterationHashSet.h"

template<UINT64 heapSizeInBytes, class Chunk>
static void allocateChunk(ResizingArray<Chunk>& chunks, ID3D12Device* graphicsDevice)
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

template<class IndexedList, class Chunk>
static void removeFromList(IndexedList& list, Chunk* chunks, Chunk& chunkToRemove, decltype(chunkToRemove.nextFreeIndex) previousIndex)
{
	const auto nextIndex = chunkToRemove.nextFreeIndex;
	if (previousIndex == std::numeric_limits<decltype(previousIndex)>::max())
	{
		list.nextFreeIndex = nextIndex;
	}
	else
	{
		chunks[previousIndex].nextFreeIndex = nextIndex;
	}
}

template<class IndexedList, class Chunk>
static void listPopBack(IndexedList& list, Chunk* chunks)
{
	assert(list.nextFreeIndex != std::numeric_limits<decltype(list.nextFreeIndex)>::max() && "can't pop an empty list");
	list.nextFreeIndex = chunks[list.nextFreeIndex].nextFreeIndex;
}

template<class IndexedList, class Chunk>
static void listPushBack(IndexedList& list, Chunk* chunks, decltype(list.nextFreeIndex) chunkIndex)
{
	auto& chunk = chunks[chunkIndex];
	chunk.nextFreeIndex = list.nextFreeIndex;
	list.nextFreeIndex = chunkIndex;
}

template<UINT64 heapSizeInBytes, class IndexedList, class Chunk>
static unsigned long findOrMakeFirstFreeChunk(unsigned long currentChunkIndex, IndexedList& freeChunks, ResizingArray<Chunk>& chunks, ID3D12Device* graphicsDevice)
{
	if (currentChunkIndex == std::numeric_limits<decltype(currentChunkIndex)>::max())
	{
		allocateChunk<heapSizeInBytes>(chunks, graphicsDevice);
		unsigned long chunkIndex = static_cast<unsigned long>(chunks.size()) - 1u;
		listPushBack(freeChunks, chunks.data(), chunkIndex);
		return chunkIndex;
	}
	return currentChunkIndex;
}

template<UINT64 heapSizeInBytes, unsigned short heapSizeInPages, class IndexedList, class Chunk>
static void allocatePage(IndexedList& freeChunks, ResizingArray<Chunk>& chunks, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, ID3D12Resource* resource,
	unsigned char resourceId, std::size_t& lastIndex, std::size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, GpuHeapLocation& heapLocation, UINT* heapOffsets, UINT* heapTileCounts)
{
	auto currentChunkIndex = findOrMakeFirstFreeChunk<heapSizeInBytes>(freeChunks.nextFreeIndex, freeChunks, chunks, graphicsDevice);
	auto streakIndex = currentIndex - lastIndex;
	auto& currentChunk = chunks[currentChunkIndex];
	heapOffsets[streakIndex] = currentChunk.freeListIndex;
	auto& pageInfo = currentChunk.pageInfos[currentChunk.freeListIndex];
	currentChunk.freeListIndex = pageInfo.nextIndex;
	pageInfo.textureId = resourceId;
	pageInfo.mipLevel = static_cast<unsigned char>(locations[currentIndex].Subresource);
	pageInfo.x = static_cast<unsigned short>(locations[currentIndex].X);
	pageInfo.y = static_cast<unsigned short>(locations[currentIndex].Y);

	heapLocation.heapIndex = currentChunkIndex;
	heapLocation.heapOffsetInPages = static_cast<unsigned short>(heapOffsets[streakIndex]);

	if (currentChunk.freeListIndex == heapSizeInPages)
	{
		UINT streakLength = static_cast<UINT>(streakIndex + 1u);
		commandQueue->UpdateTileMappings(resource, streakLength, locations + lastIndex, nullptr, currentChunk.data, streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex + 1u;
		listPopBack(freeChunks, chunks.data());
	}
}

template<unsigned short heapSizeInPages, class Chunk>
static bool hasEnoughFreePages(Chunk& chunk, std::size_t pageCount)
{
	auto freeListIndex = chunk.freeListIndex;
	while (pageCount != 0u)
	{
		if (freeListIndex == heapSizeInPages)
		{
			return false;
		}
		freeListIndex = chunk.pageInfos[freeListIndex].nextIndex;
		--pageCount;
	}
	return true;
}

void PageAllocator::allocatePageHelper(IndexedList& freeChunks, ResizingArray<Chunk>& allocatedChunks, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue,
	ID3D12Resource* resource, unsigned char resourceId, std::size_t& lastIndex, std::size_t currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, GpuHeapLocation& heapLocation,
	UINT* heapOffsets, UINT* heapTileCounts)
{
	allocatePage<heapSizeInBytes, heapSizeInPages>(freeChunks, allocatedChunks, graphicsDevice, commandQueue, resource,
		resourceId, lastIndex, currentIndex, locations, heapLocation, heapOffsets, heapTileCounts);
}

void PageAllocator::addPackedPages(ID3D12Resource* resource, GpuHeapLocation* pinnedHeapLocations, unsigned int lowestPinnedMip, std::size_t numPages, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice)
{
	assert(numPages <= heapSizeInPages && "All packed pages of a resource must go in the same heap");
	UINT heapOffsets[heapSizeInPages];
	UINT heapTileCounts[heapSizeInPages];
	auto previousChunkIndex = std::numeric_limits<decltype(freePackedChunks.nextFreeIndex)>::max();
	auto currentChunkIndex = findOrMakeFirstFreeChunk<heapSizeInBytes>(freePackedChunks.nextFreeIndex, freePackedChunks, allocatedPackedChunks, graphicsDevice);
	while (!hasEnoughFreePages<heapSizeInPages>(allocatedPackedChunks[currentChunkIndex], numPages))
	{
		previousChunkIndex = currentChunkIndex;
		currentChunkIndex = findOrMakeFirstFreeChunk<heapSizeInBytes>(allocatedPackedChunks[currentChunkIndex].nextFreeIndex, freePackedChunks, allocatedPackedChunks, graphicsDevice);
	}

	D3D12_TILED_RESOURCE_COORDINATE location;
	location.X = 0u;
	location.Y = 0u;
	location.Z = 0u;
	location.Subresource = lowestPinnedMip;

	D3D12_TILE_REGION_SIZE tileRegion;
	tileRegion.NumTiles = (UINT)numPages;
	tileRegion.UseBox = FALSE;
	
	auto& currentChunk = allocatedPackedChunks[currentChunkIndex];
	for (std::size_t i = 0u; i != numPages; ++i)
	{
		heapTileCounts[i] = 1u;
		heapOffsets[i] = currentChunk.freeListIndex;
		auto& pageInfo = currentChunk.pageInfos[currentChunk.freeListIndex];
		currentChunk.freeListIndex = pageInfo.nextIndex;

		auto& heapLocation = pinnedHeapLocations[i];
		heapLocation.heapIndex = currentChunkIndex;
		heapLocation.heapOffsetInPages = static_cast<unsigned short>(heapOffsets[i]);
	}
	if (currentChunk.freeListIndex == heapSizeInPages)
	{
		//currentChunk has run out of pages, remove it from the list of free chunks
		removeFromList(freePackedChunks, allocatedPackedChunks.data(), currentChunk, previousChunkIndex);
	}

	commandQueue->UpdateTileMappings(resource, (UINT)1u, &location, &tileRegion, currentChunk.data, (UINT)numPages,
		nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageAllocator::addPinnedPages(D3D12_TILED_RESOURCE_COORDINATE* locations, std::size_t pageCount, ID3D12Resource* resource, unsigned char resourceId,
	GpuHeapLocation* pinnedHeapLocations, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice)
{
	UINT heapOffsets[heapSizeInPages];
	UINT heapTileCounts[heapSizeInPages];
	pinnedPageCount += pageCount;

	std::size_t lastIndex = 0u;
	for (std::size_t i = 0u; i != pageCount; ++i)
	{
		heapTileCounts[i] = 1u;
		allocatePage<heapSizeInBytes, heapSizeInPages>(freeChunks, allocatedChunks, graphicsDevice, commandQueue, resource,
			resourceId, lastIndex, i, locations, pinnedHeapLocations[i], heapOffsets, heapTileCounts);
	}

	auto streakLength = static_cast<UINT>(pageCount - lastIndex);
	if (streakLength != 0u)
	{
		auto currentChunkIndex = freeChunks.nextFreeIndex;
		commandQueue->UpdateTileMappings(resource, streakLength, locations + lastIndex, nullptr, allocatedChunks[currentChunkIndex].data, streakLength,
			nullptr, heapOffsets, heapTileCounts, D3D12_TILE_MAPPING_FLAG_NONE);
	}
}

void PageAllocator::removePages(const GpuHeapLocation* heapLocations, const std::size_t pageCount)
{
	//free unused heap pages
	for (std::size_t i = 0u; i != pageCount; ++i)
	{
		const auto& heapLocation = heapLocations[i];
		auto& chunk = allocatedChunks[heapLocation.heapIndex];
		if (chunk.freeListIndex == heapSizeInPages)
		{
			listPushBack(freeChunks, allocatedChunks.data(), heapLocation.heapIndex);
		}
		chunk.pageInfos[heapLocation.heapOffsetInPages].nextIndex = chunk.freeListIndex;
		chunk.freeListIndex = heapLocation.heapOffsetInPages;
	}
}

void PageAllocator::removePage(const GpuHeapLocation heapLocation)
{
	auto& chunk = allocatedChunks[heapLocation.heapIndex];
	if (chunk.freeListIndex == heapSizeInPages)
	{
		listPushBack(freeChunks, allocatedChunks.data(), heapLocation.heapIndex);
	}
	chunk.pageInfos[heapLocation.heapOffsetInPages].nextIndex = chunk.freeListIndex;
	chunk.freeListIndex = heapLocation.heapOffsetInPages;
}

void PageAllocator::removePinnedPages(const GpuHeapLocation* pinnedHeapLocations, std::size_t numPinnedPages)
{
	//free unused heap pages
	assert(pinnedPageCount >= numPinnedPages);
	pinnedPageCount -= numPinnedPages;
	for (std::size_t i = 0u; i != numPinnedPages; ++i)
	{
		auto& heapLocation = pinnedHeapLocations[i];
		auto& chunk = allocatedChunks[heapLocation.heapIndex];
		if (chunk.freeListIndex == heapSizeInPages)
		{
			listPushBack(freeChunks, allocatedChunks.data(), heapLocation.heapIndex);
		}
		chunk.pageInfos[heapLocation.heapOffsetInPages].nextIndex = chunk.freeListIndex;
		chunk.freeListIndex = heapLocation.heapOffsetInPages;
	}
}

void PageAllocator::removePackedPages(const GpuHeapLocation* pinnedHeapLocations, std::size_t numPinnedPages)
{
	for (std::size_t i = 0u; i != numPinnedPages; ++i)
	{
		auto& heapLocation = pinnedHeapLocations[i];
		auto& chunk = allocatedPackedChunks[heapLocation.heapIndex];
		if (chunk.freeListIndex == heapSizeInPages)
		{
			listPushBack(freePackedChunks, allocatedPackedChunks.data(), heapLocation.heapIndex);
		}
		chunk.pageInfos[heapLocation.heapOffsetInPages].nextIndex = chunk.freeListIndex;
		chunk.freeListIndex = heapLocation.heapOffsetInPages;
	}
}

namespace
{
	//Delays deleting the heap and resource until they aren't in use anymore
	struct ChunkDeleteRequest : public GraphicsEngine::Task
	{
		D3D12Heap heap;
		D3D12Resource resource;
		std::size_t chunkCount;
	};

	ChunkDeleteRequest* createChunkDeleteRequests(std::size_t chunkCount)
	{
		auto* requests = new ChunkDeleteRequest[chunkCount];
		requests->execute = [](GraphicsEngine::Task& task, void*)
		{
			ChunkDeleteRequest& request = static_cast<ChunkDeleteRequest&>(task);
			delete[] &request;
		};
		return requests;
	}
}

struct PageAllocator::ResourceDeleteInfo
{
	unsigned char id;
	unsigned short pageInfos = heapSizeInPages;

	ResourceDeleteInfo(unsigned char id1) : id(id1) {}

	struct Hasher
	{
		std::size_t operator()(const PageAllocator::ResourceDeleteInfo& info) const noexcept
		{
			return std::size_t{ info.id };
		}
	};

	bool operator==(const PageAllocator::ResourceDeleteInfo& other) const noexcept
	{
		return id == other.id;
	}

	bool operator!=(const PageAllocator::ResourceDeleteInfo& other) const noexcept
	{
		return id != other.id;
	}
};

void PageAllocator::decreaseNonPinnedCapacity(std::size_t newSize, VirtualTextureInfoByID& texturesById, GraphicsEngine& graphicsEngine, ID3D12GraphicsCommandList& commandList)
{
	std::size_t newNumChunks = (newSize + pinnedPageCount + heapSizeInPages - 1u) / heapSizeInPages;
	if (allocatedChunks.size() <= newNumChunks) return;
	const std::size_t numChunksToRemove = allocatedChunks.size() - newNumChunks;

	IndexedList chunksToRemove{ std::numeric_limits<unsigned long>::max() };
	for (auto i = numChunksToRemove; i != 0u; --i)
	{
		auto chunkToRemoveIndex = freeChunks.nextFreeIndex;
		listPopBack(freeChunks, allocatedChunks.data());
		listPushBack(chunksToRemove, allocatedChunks.data(), chunkToRemoveIndex);
	}

	auto* chunkDeleteRequests = createChunkDeleteRequests(numChunksToRemove);
	for (std::size_t i = 0u; i != numChunksToRemove; ++i)
	{
		chunkDeleteRequests[i].heap = std::move(allocatedChunks[newNumChunks + i].data);
		chunkDeleteRequests[i].resource = D3D12Resource(graphicsEngine.graphicsDevice, chunkDeleteRequests[i].heap, 0u, []()
			{
				D3D12_RESOURCE_DESC desc;
				desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				desc.Width = heapSizeInBytes;
				desc.Height = 1u;
				desc.DepthOrArraySize = 1u;
				desc.MipLevels = 1u;
				desc.Format = DXGI_FORMAT_UNKNOWN;
				desc.SampleDesc = { 1u, 0u };
				desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				desc.Flags = D3D12_RESOURCE_FLAG_NONE;
				return desc;
			}(),
				D3D12_RESOURCE_STATE_COPY_SOURCE);

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrier.Aliasing.pResourceBefore = nullptr;
		barrier.Aliasing.pResourceAfter = chunkDeleteRequests[i].resource;
		commandList.ResourceBarrier(1u, &barrier);
	}

	FixedCapacityFastIterationHashSet<unsigned char, 255u> uniqueResourcesToRemove;
	FixedCapacityFastIterationHashSet<ResourceDeleteInfo, 255u, ResourceDeleteInfo::Hasher> resourcesToRemove;
	std::size_t chunkDeleteRequestIndex = 0u;
	for (auto i = chunksToRemove.nextFreeIndex; i != std::numeric_limits<decltype(chunksToRemove.nextFreeIndex)>::max(); i = allocatedChunks[i].nextFreeIndex)
	{
		ID3D12Heap* currentHeap = chunkDeleteRequests[chunkDeleteRequestIndex].heap;
		ID3D12Resource* resourceForCopyingTiles = chunkDeleteRequests[chunkDeleteRequestIndex].resource;
		auto& chunkToRemove = allocatedChunks[i];
		std::bitset<heapSizeInPages> pageIsFree{};
		auto& pageInfos = chunkToRemove.pageInfos;
		for (auto freeListIndex = chunkToRemove.freeListIndex; freeListIndex != heapSizeInPages; freeListIndex = pageInfos[freeListIndex].nextIndex)
		{
			pageIsFree.set(freeListIndex);
		}
		for (unsigned short allocatedListIndex = 0u; allocatedListIndex != heapSizeInPages; ++allocatedListIndex)
		{
			if (pageIsFree[allocatedListIndex]) continue;
			auto& pageInfo = pageInfos[allocatedListIndex];
			auto& resourceDeleteInfo = resourcesToRemove[pageInfo.textureId];
			pageInfo.nextIndex = resourceDeleteInfo.pageInfos;
			resourceDeleteInfo.pageInfos = allocatedListIndex;
		}
		{
			D3D12_RESOURCE_BARRIER barriers[255u];
			unsigned int barrierCount = 0u;
			for (const auto& resourceDeleteInfo : resourcesToRemove)
			{
				bool inserted = uniqueResourcesToRemove.insertOrReplace(resourceDeleteInfo.id);
				if (inserted)
				{
					barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barriers[barrierCount].Transition.pResource = texturesById[resourceDeleteInfo.id].resource;
					barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[barrierCount].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					barriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
					++barrierCount;
				}
			}
			if (barrierCount != 0u)
			{
				commandList.ResourceBarrier(barrierCount, barriers);
			}
		}
		resourcesToRemove.consume([&](const ResourceDeleteInfo& resourceDeleteInfo)
			{
				VirtualTextureInfo& textureInfo = texturesById[resourceDeleteInfo.id];

				D3D12_TILED_RESOURCE_COORDINATE destinationCoordinates[heapSizeInPages];
				UINT heapOffsets[heapSizeInPages];
				UINT pageCount = 1u;
				for (unsigned short currentPageInfoIndex = resourceDeleteInfo.pageInfos; currentPageInfoIndex != heapSizeInPages; currentPageInfoIndex = pageInfos[currentPageInfoIndex].nextIndex)
				{
					auto& pageInfo = pageInfos[currentPageInfoIndex];

					destinationCoordinates[pageCount].X = pageInfo.x;
					destinationCoordinates[pageCount].Y = pageInfo.y;
					destinationCoordinates[pageCount].Z = 0u;
					destinationCoordinates[pageCount].Subresource = pageInfo.mipLevel;

					const auto freeChunkIndex = freeChunks.nextFreeIndex;
					auto& freeChunk = allocatedChunks[freeChunkIndex];
					heapOffsets[pageCount] = freeChunk.freeListIndex;
					auto& newPageInfo = freeChunk.pageInfos[freeChunk.freeListIndex];
					freeChunk.freeListIndex = newPageInfo.nextIndex;
					newPageInfo.textureId = pageInfo.textureId;
					newPageInfo.mipLevel = pageInfo.mipLevel;
					newPageInfo.x = pageInfo.x;
					newPageInfo.y = pageInfo.y;
					if (freeChunk.freeListIndex == heapSizeInPages)
					{
						listPopBack(freeChunks, allocatedChunks.data());
					}
					auto pageLookUpData = textureInfo.pageCacheData.pageLookUp.find(PageResourceLocation{ pageInfo.textureId, pageInfo.mipLevel, pageInfo.x, pageInfo.y });
					pageLookUpData->data.heapLocation = GpuHeapLocation{ freeChunkIndex, static_cast<unsigned short>(heapOffsets[pageCount]) };

					++pageCount;
				}

				graphicsEngine.directCommandQueue->UpdateTileMappings(textureInfo.resource, 1u, destinationCoordinates, nullptr, currentHeap, 1u,
					nullptr, heapOffsets, &pageCount, D3D12_TILE_MAPPING_FLAG_NONE);

				const D3D12_TILE_REGION_SIZE regionSize{ 1u, TRUE, 1u, 1u, 1u };
				unsigned short currentPageInfoIndex = resourceDeleteInfo.pageInfos;
				for (UINT j = 0u; j != pageCount; ++j)
				{
					UINT64 bufferOffsetInBytes = currentPageInfoIndex * pageSizeInBytes;
					commandList.CopyTiles(textureInfo.resource, &destinationCoordinates[j], &regionSize, resourceForCopyingTiles, bufferOffsetInBytes, D3D12_TILE_COPY_FLAG_NONE);
					currentPageInfoIndex = pageInfos[currentPageInfoIndex].nextIndex;
				}
			});

		++chunkDeleteRequestIndex;
	}
	graphicsEngine.executeWhenGpuFinishesCurrentFrame(*chunkDeleteRequests);
	{
		D3D12_RESOURCE_BARRIER barriers[255u];
		unsigned int barrierCount = 0u;
		for (auto resourceId : uniqueResourcesToRemove)
		{
			barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barriers[barrierCount].Transition.pResource = texturesById[resourceId].resource;
			barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barriers[barrierCount].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			++barrierCount;
		}
		commandList.ResourceBarrier(barrierCount, barriers);
	}
	for (std::size_t i = 0u; i != numChunksToRemove; ++i)
	{
		allocatedChunks.pop_back();
	}
}

std::size_t PageAllocator::nonPinnedMemoryUsageInPages()
{
	return allocatedChunks.size() * heapSizeInPages - pinnedPageCount;
}