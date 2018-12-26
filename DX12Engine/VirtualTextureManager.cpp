#include "VirtualTextureManager.h"

void VirtualTextureManager::loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest, StreamingManager& streamingManager, D3D12GraphicsEngine& graphicsEngine,
	void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
	void(*resourceUploaded)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
	const DDSFileLoader::DdsHeaderDx12& header)
{
	bool valid = DDSFileLoader::validateDdsHeader(header);
	if (!valid) throw false;

	uploadRequest.streamResource = useSubresource;
	uploadRequest.resourceUploaded = resourceUploaded;
	uploadRequest.width = header.width;
	uploadRequest.height = header.height;
	uploadRequest.format = header.dxgiFormat;
	uploadRequest.mipLevels = (uint16_t)header.mipMapCount;
	uploadRequest.dimension = (D3D12_RESOURCE_DIMENSION)header.dimension;
	uploadRequest.depth = (uint16_t)header.depth;
	assert(header.depth == 1u);
	assert(header.arraySize == 1u);
	uploadRequest.destResource = createTextureWithResitencyInfo(graphicsEngine, streamingManager.commandQueue(), uploadRequest);

	auto width = header.width >> uploadRequest.mostDetailedMip;
	if(width == 0u) width = 1u;
	auto height = header.height >> uploadRequest.mostDetailedMip;
	if(height == 0u) height = 1u;
	std::size_t numBytes, numRows, rowBytes;
	DDSFileLoader::surfaceInfo(width, height, header.dxgiFormat, numBytes, rowBytes, numRows);
	uploadRequest.resourceSize = (unsigned long)numBytes;
}

D3D12Resource VirtualTextureManager::createTexture(ID3D12Device* graphicsDevice, const TextureStreamingRequest& request)
{
	D3D12_RESOURCE_DESC textureDesc;
	textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	textureDesc.DepthOrArraySize = request.depth;
	textureDesc.Dimension = request.dimension;
	textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	textureDesc.Format = request.format;
	textureDesc.Height = request.height;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		
	textureDesc.MipLevels = request.mipLevels;
	textureDesc.SampleDesc.Count = 1u;
	textureDesc.SampleDesc.Quality = 0u;
	textureDesc.Width = request.width;

	return D3D12Resource(graphicsDevice, textureDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
}

unsigned int VirtualTextureManager::createTextureDescriptor(D3D12GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request)
{
	auto discriptorIndex = graphicsEngine.descriptorAllocator.allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE discriptorHandle = graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		discriptorIndex * graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv;
	srv.Format = request.format;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (request.dimension)
	{
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
		srv.Texture3D.MipLevels = request.mipLevels;
		srv.Texture3D.MostDetailedMip = 0u;
		srv.Texture3D.ResourceMinLODClamp = 0u;
		break;
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = request.mipLevels;
		srv.Texture2D.MostDetailedMip = 0u;
		srv.Texture2D.ResourceMinLODClamp = 0u;
		srv.Texture2D.PlaneSlice = 0u;
		break;
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
		srv.Texture1D.MipLevels = request.mipLevels;
		srv.Texture1D.MostDetailedMip = 0u;
		srv.Texture1D.ResourceMinLODClamp = 0u;
		break;
	}

	graphicsEngine.graphicsDevice->CreateShaderResourceView(texture, &srv, discriptorHandle);
	return discriptorIndex;
}

void VirtualTextureManager::unloadTextureHelper(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, StreamingManager& streamingManager)
{
	unsigned int descriptorIndex = std::numeric_limits<unsigned int>::max();
	unsigned int textureID = 255;
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto& texture = textures[filename];
		texture.numUsers -= 1u;
		if (texture.numUsers == 0u)
		{
			descriptorIndex = texture.descriptorIndex;
			textureID = texture.textureID;
			textures.erase(filename);
		}
	}
	if (descriptorIndex != std::numeric_limits<unsigned int>::max())
	{
		graphicsEngine.descriptorAllocator.deallocate(descriptorIndex);
		assert(textureID != 255);
		auto& resitencyInfo = texturesByID[textureID];

		D3D12_PACKED_MIP_INFO packedMipInfo;
		D3D12_SUBRESOURCE_TILING subresourceTiling;
		graphicsEngine.graphicsDevice->GetResourceTiling(resitencyInfo.resource, nullptr, &packedMipInfo, nullptr, nullptr, 0u, &subresourceTiling);
		if (packedMipInfo.NumPackedMips != 0u)
		{
			pageProvider.pageAllocator.removePinnedPages(resitencyInfo.pinnedHeapLocations, packedMipInfo.NumTilesForPackedMips);

			D3D12_TILED_RESOURCE_COORDINATE resourceTileCoord;
			resourceTileCoord.X = 0u;
			resourceTileCoord.Y = 0u;
			resourceTileCoord.Z = 0u;
			resourceTileCoord.Subresource = resitencyInfo.lowestPinnedMip;

			D3D12_TILE_REGION_SIZE tileRegion;
			tileRegion.NumTiles = packedMipInfo.NumTilesForPackedMips;
			tileRegion.UseBox = FALSE;

			D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
			streamingManager.commandQueue().UpdateTileMappings(resitencyInfo.resource, 1u, &resourceTileCoord, &tileRegion, nullptr, 1u, &rangeFlags, nullptr,
				nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		}
		else
		{
			auto width = resitencyInfo.widthInPages >> resitencyInfo.lowestPinnedMip;
			if (width == 0u) width = 1u;
			auto height = resitencyInfo.heightInPages >> resitencyInfo.lowestPinnedMip;
			if (height == 0u) height = 1u;
			auto numPages = width * height;
			pageProvider.pageAllocator.removePinnedPages(resitencyInfo.pinnedHeapLocations, numPages);

			D3D12_TILED_RESOURCE_COORDINATE resourceTileCoord;
			resourceTileCoord.X = 0u;
			resourceTileCoord.Y = 0u;
			resourceTileCoord.Z = 0u;
			resourceTileCoord.Subresource = resitencyInfo.lowestPinnedMip;

			D3D12_TILE_REGION_SIZE tileRegion;
			tileRegion.NumTiles = numPages;
			tileRegion.UseBox = TRUE;
			tileRegion.Width = (uint32_t)width;
			tileRegion.Height = (uint16_t)height;
			tileRegion.Depth = 1u;

			D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
			streamingManager.commandQueue().UpdateTileMappings(resitencyInfo.resource, 1u, &resourceTileCoord, &tileRegion, nullptr, 1u, &rangeFlags, nullptr,
				nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		}
		
		delete[] resitencyInfo.pinnedHeapLocations;
		resitencyInfo.resource->Release();
		{
			std::lock_guard<decltype(mutex)> lock(mutex);
			texturesByID.deallocate(textureID);
		}
	}
}

ID3D12Resource* VirtualTextureManager::createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue& commandQueue, TextureStreamingRequest& vramRequest)
{
	D3D12Resource resource = createTexture(graphicsEngine.graphicsDevice, vramRequest);
#ifndef NDEBUG
	std::wstring name = L"virtual texture ";
	name += vramRequest.filename;
	resource->SetName(name.c_str());
#endif
	D3D12_PACKED_MIP_INFO packedMipInfo;
	D3D12_TILE_SHAPE tileShape;
	D3D12_SUBRESOURCE_TILING subresourceTiling;
	UINT numSubresourceTilings = 1u;
	graphicsEngine.graphicsDevice->GetResourceTiling(resource, nullptr, &packedMipInfo, &tileShape, &numSubresourceTilings, 0u, &subresourceTiling);
	unsigned int textureDescriptorIndex = createTextureDescriptor(graphicsEngine, resource, vramRequest);

	VirtualTextureInfo resitencyInfo;
	resitencyInfo.width = vramRequest.width;
	resitencyInfo.height = vramRequest.height;
	resitencyInfo.file = vramRequest.file;
	resitencyInfo.filename = vramRequest.filename;
	resitencyInfo.widthInPages = subresourceTiling.WidthInTiles;
	resitencyInfo.heightInPages = subresourceTiling.HeightInTiles;
	resitencyInfo.numMipLevels = vramRequest.mipLevels;
	resitencyInfo.resource = resource;
	resitencyInfo.format = vramRequest.format;
	if (packedMipInfo.NumPackedMips == 0u)
	{
		//Pin lowest mip

		resitencyInfo.lowestPinnedMip = vramRequest.mipLevels - 1u;

		D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[64];
		constexpr size_t resourceTileCoordsMax = sizeof(resourceTileCoords) / sizeof(resourceTileCoords[0u]);
		unsigned int resourceTileCoordsIndex = 0u;

		auto width = resitencyInfo.widthInPages >> resitencyInfo.lowestPinnedMip;
		if (width == 0u) width = 1u;
		auto height = resitencyInfo.heightInPages >> resitencyInfo.lowestPinnedMip;
		if (height == 0u) height = 1u;
		resitencyInfo.pinnedHeapLocations = new HeapLocation[width * height];
		for (auto y = 0u; y < height; ++y)
		{
			for (auto x = 0u; x < width; ++x)
			{
				if (resourceTileCoordsIndex == resourceTileCoordsMax)
				{
					pageProvider.pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsMax, resitencyInfo, &commandQueue, graphicsEngine.graphicsDevice);
					resourceTileCoordsIndex = 0u;
				}
				resourceTileCoords[resourceTileCoordsIndex].X = x;
				resourceTileCoords[resourceTileCoordsIndex].Y = y;
				resourceTileCoords[resourceTileCoordsIndex].Z = 0u;
				resourceTileCoords[resourceTileCoordsIndex].Subresource = resitencyInfo.lowestPinnedMip;
				++resourceTileCoordsIndex;
			}
		}
		pageProvider.pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsIndex, resitencyInfo, &commandQueue, graphicsEngine.graphicsDevice);
	}
	else
	{
		resitencyInfo.pinnedHeapLocations = new HeapLocation[packedMipInfo.NumPackedMips];
		resitencyInfo.lowestPinnedMip = vramRequest.mipLevels - packedMipInfo.NumPackedMips;
		pageProvider.pageAllocator.addPackedPages(resitencyInfo, packedMipInfo.NumTilesForPackedMips, &commandQueue, graphicsEngine.graphicsDevice);
	}
	vramRequest.mostDetailedMip = (uint16_t)resitencyInfo.lowestPinnedMip;
	ID3D12Resource* resourcePtr = resource;

	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		auto& textureInfo = textures[vramRequest.filename];
		textureInfo.resource = resource.steal();
		textureInfo.descriptorIndex = textureDescriptorIndex;
		unsigned int textureID = texturesByID.allocate();
		textureInfo.textureID = textureID;
		texturesByID[textureID] = std::move(resitencyInfo);
	}

	return resourcePtr;
}

void VirtualTextureManager::textureUploadedHelper(const wchar_t* filename, void* tr, void* gr)
{
	TextureStreamingRequest* requests;
	Texture* texture;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		texture = &textures[filename];
		texture->loaded = true;

		auto request = uploadRequests.find(filename);
		assert(request != uploadRequests.end() && "A texture is loading with no requests for it");
		requests = std::move(request->second);
		uploadRequests.erase(request);
	}
	for(auto current = requests; current != nullptr; )
	{
		auto old = current;
		current = current->nextTextureRequest; //Need to do this now as old will be deleted by the next line
		old->textureLoaded(*old, tr, gr, *texture);
	}
}

void VirtualTextureManager::textureUseResourceHelper(TextureStreamingRequest& uploadRequest, void(*fileLoadedCallback)(AsynchronousFileManager::IORequest& request, void* tr, void*, const unsigned char* buffer))
{
	std::size_t subresouceWidth = uploadRequest.width;
	std::size_t subresourceHeight = uploadRequest.height;
	std::size_t subresourceDepth = uploadRequest.depth;
	std::size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

	for(std::size_t currentMip = 0u; currentMip != uploadRequest.mostDetailedMip; ++currentMip)
	{
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.format, numBytes, rowBytes, numRows);
		fileOffset += numBytes * subresourceDepth;

		subresouceWidth >>= 1u;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		subresourceHeight >>= 1u;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
		subresourceDepth >>= 1u;
		if(subresourceDepth == 0u) subresourceDepth = 1u;
	}

	std::size_t subresourceSize = 0u;
	for(std::size_t currentMip = uploadRequest.mostDetailedMip; currentMip != uploadRequest.mipLevels; ++currentMip)
	{
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.format, numBytes, rowBytes, numRows);
		subresourceSize += numBytes * subresourceDepth;

		subresouceWidth >>= 1u;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		subresourceHeight >>= 1u;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
		subresourceDepth >>= 1u;
		if(subresourceDepth == 0u) subresourceDepth = 1u;
	}



	uploadRequest.start = fileOffset;
	uploadRequest.end = fileOffset + subresourceSize;
	uploadRequest.fileLoadedCallback = fileLoadedCallback;
}

void VirtualTextureManager::fileLoadedCallbackHelper(TextureStreamingRequest& uploadRequest, const unsigned char* buffer, StreamingManager::ThreadLocal& streamingManager,
	void(*copyStarted)(void* requester, void* tr, void* gr))
{
	ID3D12Resource* destResource = uploadRequest.destResource;

	unsigned long uploadResourceOffset = uploadRequest.uploadResourceOffset;
	unsigned char* uploadBufferCurrentCpuAddress = uploadRequest.uploadBufferCurrentCpuAddress;
	for(uint32_t i = uploadRequest.mostDetailedMip;;)
	{
		uint32_t subresouceWidth = uploadRequest.width >> uploadRequest.mostDetailedMip;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		uint32_t subresourceHeight = uploadRequest.height >> uploadRequest.mostDetailedMip;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
		uint32_t subresourceDepth = uploadRequest.depth >> uploadRequest.mostDetailedMip;
		if(subresourceDepth == 0u) subresourceDepth = 1u;

		DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, uploadResourceOffset, subresouceWidth, subresourceHeight,
			subresourceDepth, uploadRequest.mostDetailedMip, uploadRequest.mipLevels, 0u, uploadRequest.format,
			uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());

		++i;
		if(i == uploadRequest.mipLevels) break;

		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.format, numBytes, rowBytes, numRows);
		std::size_t alignedSize = (rowBytes + (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) * numRows * subresourceDepth;
		uploadResourceOffset += (unsigned long)alignedSize;
		uploadBufferCurrentCpuAddress += alignedSize;
	}

	uint32_t subresouceWidth = uploadRequest.width >> uploadRequest.mostDetailedMip;
	if(subresouceWidth == 0u) subresouceWidth = 1u;
	uint32_t subresourceHeight = uploadRequest.height >> uploadRequest.mostDetailedMip;
	if(subresourceHeight == 0u) subresourceHeight = 1u;
	uint32_t subresourceDepth = uploadRequest.depth >> uploadRequest.mostDetailedMip;
	if(subresourceDepth == 0u) subresourceDepth = 1u;

	DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, subresouceWidth, subresourceHeight,
		subresourceDepth, uploadRequest.mostDetailedMip, uploadRequest.mipLevels, 0u, uploadRequest.format,
		uploadRequest.uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());
	streamingManager.addCopyCompletionEvent(&uploadRequest, copyStarted);
}