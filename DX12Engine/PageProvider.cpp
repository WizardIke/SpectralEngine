#include "PageProvider.h"

void PageProvider::allocateChunk(ID3D12Device* graphicsDevice)
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

void PageProvider::findOrMakeFirstFreeChunk(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice)
{
	while (true)
	{
		if (currentChunk == chunksEnd)
		{
			auto index = currentChunk - chunks.begin();
			allocateChunk(graphicsDevice);
			currentChunk = chunks.begin() + index;
			chunksEnd = chunks.end();
			break;
		}
		else if (currentChunk->freeList != currentChunk->freeListEnd)
		{
			break;
		}
	}
}

unsigned long PageProvider::resourceLocationToHeapLocationIndex(const D3D12_TILED_RESOURCE_COORDINATE& resourceLocation, const TextureResitency& textureInfo)
{
	unsigned long heapLocationIndex = 0u;
	auto subresourceWidth = textureInfo.widthInPages;
	auto subresourceHeight = textureInfo.heightInPages;
	for (auto i = 0u; i < resourceLocation.Subresource; ++i)
	{
		heapLocationIndex += subresourceWidth * subresourceHeight;

		subresourceWidth >>= 1u;
		if (subresourceWidth == 0u) subresourceWidth = 1u;
		subresourceHeight >>= 1u;
		if (subresourceHeight == 0u) subresourceHeight = 1u;
	}
	heapLocationIndex += resourceLocation.Y * subresourceWidth + resourceLocation.X;
	return heapLocationIndex;
}

void PageProvider::allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo,
	unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
			flags + lastIndex, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	unsigned long heapLocationIndex = resourceLocationToHeapLocationIndex(locations[currentIndex], textureInfo);
	auto& heapLocation = textureInfo.heapLocations[heapLocationIndex];
	heapLocation.heapIndex = (unsigned int)(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageProvider::allocatePage(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo,
	unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILED_RESOURCE_COORDINATE* locations)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
			nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	unsigned long heapLocationIndex = resourceLocationToHeapLocationIndex(locations[currentIndex], textureInfo);
	auto& heapLocation = textureInfo.heapLocations[heapLocationIndex];
	heapLocation.heapIndex = static_cast<unsigned int>(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageProvider::allocatePagePacked(decltype(chunks.begin())& currentChunk, decltype(chunks.end())& chunksEnd, ID3D12Device* graphicsDevice, ID3D12CommandQueue* commandQueue, const TextureResitency& textureInfo,
	unsigned int& lastIndex, unsigned int currentIndex, D3D12_TILE_REGION_SIZE& tileRegion, const D3D12_TILED_RESOURCE_COORDINATE& location, HeapLocation* heapLocations)
{
	auto streakLength = currentIndex - lastIndex;
	if (currentChunk->freeList == currentChunk->freeListEnd)
	{
		tileRegion.NumTiles = streakLength;
		commandQueue->UpdateTileMappings(textureInfo.resource, 1u, &location, &tileRegion, currentChunk->data, streakLength,
			nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		lastIndex = currentIndex;
		findOrMakeFirstFreeChunk(currentChunk, chunksEnd, graphicsDevice);
	}

	--(currentChunk->freeListEnd);
	heapOffsets[streakLength] = *currentChunk->freeListEnd;

	auto& heapLocation = heapLocations[0];
	heapLocation.heapIndex = (unsigned int)(currentChunk - chunks.begin());
	heapLocation.heapOffsetInPages = heapOffsets[streakLength];
}

void PageProvider::addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags, unsigned int pageCount, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo, ID3D12Device* graphicsDevice)
{
	//find first free space
	auto currentChunk = chunks.begin();
	auto chunksEnd = chunks.end();
	findOrMakeFirstFreeChunk(currentChunk, chunksEnd, graphicsDevice);

	//allocate heap pages
	unsigned int lastIndex = 0u;
	const auto flagEnd = flags + pageCount;
	for (auto i = 0u; i < pageCount; ++i)
	{
		if (flags[i] == D3D12_TILE_RANGE_FLAGS::D3D12_TILE_RANGE_FLAG_NONE)
		{
			allocatePage(currentChunk, chunksEnd, graphicsDevice, commandQueue, textureInfo, lastIndex, i, locations, flags);
		}
	}
	auto streakLength = pageCount - lastIndex;
	commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
		flags + lastIndex, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageProvider::removePages(D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags, unsigned int pageCount, TextureResitency& textureInfo)
{
	//free unused heap pages
	const auto flagEnd = flags + pageCount;
	for (auto flagPtr = flags; flagPtr != flagEnd; ++flagPtr)
	{
		if (*flagPtr == D3D12_TILE_RANGE_FLAGS::D3D12_TILE_RANGE_FLAG_NULL)
		{
			const auto flagIndex = flagPtr - flags;
			unsigned long heapLocationIndex = resourceLocationToHeapLocationIndex(locations[flagIndex], textureInfo);
			auto& heapLocation = textureInfo.heapLocations[heapLocationIndex];
			*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
			++(chunks[heapLocation.heapIndex].freeListEnd);
		}
	}
}

void PageProvider::updatePages(D3D12_TILED_RESOURCE_COORDINATE* locations, D3D12_TILE_RANGE_FLAGS* flags, unsigned int pageCount, ID3D12CommandQueue* commandQueue, TextureResitency& textureInfo, ID3D12Device* graphicsDevice)
{
	removePages(locations, flags, pageCount, textureInfo);
	addPages(locations, flags, pageCount, commandQueue, textureInfo, graphicsDevice);
}


void PageProvider::addPages(D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, TextureResitency& textureInfo, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice)
{
	auto currentChunk = chunks.begin();
	auto chunksEnd = chunks.end();
	findOrMakeFirstFreeChunk(currentChunk, chunksEnd, graphicsDevice);

	unsigned int lastIndex = 0u;
	for (auto i = 0u; i < pageCount; ++i)
	{
		allocatePage(currentChunk, chunksEnd, graphicsDevice, commandQueue, textureInfo, lastIndex, i, locations);
	}

	auto streakLength = pageCount - lastIndex;
	commandQueue->UpdateTileMappings(textureInfo.resource, streakLength, locations + lastIndex, nullptr, currentChunk->data, streakLength,
		nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageProvider::addPackedPages(const TextureResitency& textureInfo, ID3D12CommandQueue* commandQueue, ID3D12Device* graphicsDevice, HeapLocation* heapLocations)
{
	auto currentChunk = chunks.begin();
	auto chunksEnd = chunks.end();
	findOrMakeFirstFreeChunk(currentChunk, chunksEnd, graphicsDevice);

	unsigned int lastIndex = 0u;
	D3D12_TILED_RESOURCE_COORDINATE location;
	location.X = 0u;
	location.Y = 0u;
	location.Z = 0u;
	location.Subresource = textureInfo.mostDetailedPackedMip;

	D3D12_TILE_REGION_SIZE tileRegion;
	tileRegion.UseBox = FALSE;

	unsigned long numPages = textureInfo.numTilesForPackedMips;
	for (auto i = 0u; i < numPages; ++i)
	{
		allocatePagePacked(currentChunk, chunksEnd, graphicsDevice, commandQueue, textureInfo, lastIndex, i, tileRegion, location, heapLocations);
		++heapLocations;
	}

	auto streakLength = numPages - lastIndex;
	tileRegion.NumTiles = streakLength;
	commandQueue->UpdateTileMappings(textureInfo.resource, 1u, &location, &tileRegion, currentChunk->data, streakLength,
		nullptr, heapOffsets, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
}

void PageProvider::removePages(const D3D12_TILED_RESOURCE_COORDINATE* locations, unsigned int pageCount, const TextureResitency& textureInfo)
{
	//free unused heap pages
	for (auto i = 0u; i < pageCount; ++i)
	{
		unsigned long heapLocationIndex = resourceLocationToHeapLocationIndex(locations[i], textureInfo);
		auto& heapLocation = textureInfo.heapLocations[heapLocationIndex];
		*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
		++(chunks[heapLocation.heapIndex].freeListEnd);
	}
}

void PageProvider::removePackedPages(const D3D12_TILED_RESOURCE_COORDINATE& location, const D3D12_TILE_REGION_SIZE& tileRegion, TextureResitency& textureInfo)
{
	//free unused heap pages
	unsigned int packedMipTileIndex = 0u;
	const auto numStandardMips = textureInfo.numMipLevels - textureInfo.mostDetailedPackedMip;
	for (auto i = 0u; i < numStandardMips; ++i)
	{
		auto widthInPages = textureInfo.widthInPages >> i;
		if (widthInPages == 0u) widthInPages = 1u;
		auto heightInPages = textureInfo.heightInPages >> i;
		if (heightInPages == 0u) heightInPages = 1u;
		packedMipTileIndex += widthInPages * heightInPages;
	}

	const auto end = packedMipTileIndex + tileRegion.NumTiles;
	for (; packedMipTileIndex != end; ++packedMipTileIndex)
	{
		auto& heapLocation = textureInfo.heapLocations[packedMipTileIndex];
		*chunks[heapLocation.heapIndex].freeListEnd = heapLocation.heapOffsetInPages;
		++(chunks[heapLocation.heapIndex].freeListEnd);
	}
}