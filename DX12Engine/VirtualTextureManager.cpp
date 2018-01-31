#include "VirtualTextureManager.h"
#include "SharedResources.h"

void VirtualTextureManager::loadTextureUncachedHelper(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, const wchar_t * filename,
	void(*textureUseSubresource)(RamToVramUploadRequest* const request, BaseExecutor* executor1, SharedResources& sharedResources, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset), void(*textureUploaded)(void* storedFilename, BaseExecutor* exe, SharedResources& sharedResources))
{
	RamToVramUploadRequest& uploadRequest = streamingManager.getUploadRequest();
	uploadRequest.useSubresourcePointer = textureUseSubresource;
	uploadRequest.requester = reinterpret_cast<void*>(const_cast<wchar_t *>(filename));
	uploadRequest.resourceUploadedPointer = textureUploaded;

	uploadRequest.file.open(filename, ScopedFile::accessRight::genericRead, ScopedFile::shareMode::readMode, ScopedFile::creationMode::openExisting, nullptr);
	DDSFileLoader::TextureInfo textureInfo = DDSFileLoader::getDDSTextureInfoFromFile(uploadRequest.file);

	uploadRequest.width = textureInfo.width;
	uploadRequest.height = textureInfo.height;
	uploadRequest.format = textureInfo.format;
	uploadRequest.mipLevels = textureInfo.mipLevels;
	uploadRequest.dimension = textureInfo.dimension;
	uploadRequest.currentArrayIndex = 0u;

	if (textureInfo.dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		uploadRequest.arraySize = 1u;
		uploadRequest.depth = textureInfo.depthOrArraySize;
	}
	else
	{
		uploadRequest.arraySize = textureInfo.depthOrArraySize;
		uploadRequest.depth = 1u;
	}

	createTextureWithResitencyInfo(graphicsEngine, commandQueue, uploadRequest, filename);
}

static D3D12Resource createTexture(ID3D12Device* graphicsDevice, const RamToVramUploadRequest& request)
{
	D3D12_RESOURCE_DESC textureDesc;
	textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	textureDesc.DepthOrArraySize = std::max((uint16_t)request.depth, request.arraySize);
	textureDesc.Dimension = request.dimension;
	textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	textureDesc.Format = request.format;
	textureDesc.Height = request.height;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		
	textureDesc.MipLevels = request.mipLevels;
	textureDesc.SampleDesc.Count = 1u;
	textureDesc.SampleDesc.Quality = 0u;
	textureDesc.Width = request.width;

	return D3D12Resource(graphicsDevice, textureDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr);
}

static unsigned int createTextureDescriptor(D3D12GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const RamToVramUploadRequest& request)
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
		if (request.arraySize > 1u)
		{
			srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srv.Texture2DArray.MipLevels = request.mipLevels;
			srv.Texture2DArray.MostDetailedMip = 0u;
			srv.Texture2DArray.ResourceMinLODClamp = 0u;
			srv.Texture2DArray.ArraySize = request.arraySize;
			srv.Texture2DArray.FirstArraySlice = 0u;
			srv.Texture2DArray.PlaneSlice = 0u;
		}
		else
		{
			srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
			srv.Texture2D.MipLevels = request.mipLevels;
			srv.Texture2D.MostDetailedMip = 0u;
			srv.Texture2D.ResourceMinLODClamp = 0u;
			srv.Texture2D.PlaneSlice = 0u;
		}
		break;
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		if (request.arraySize > 1u)
		{
			srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
			srv.Texture1DArray.MipLevels = request.mipLevels;
			srv.Texture1DArray.MostDetailedMip = 0u;
			srv.Texture1DArray.ResourceMinLODClamp = 0u;
			srv.Texture1DArray.ArraySize = request.arraySize;
			srv.Texture1DArray.FirstArraySlice = 0u;
		}
		else
		{
			srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
			srv.Texture1D.MipLevels = request.mipLevels;
			srv.Texture1D.MostDetailedMip = 0u;
			srv.Texture1D.ResourceMinLODClamp = 0u;
		}
		break;
	}

	graphicsEngine.graphicsDevice->CreateShaderResourceView(texture, &srv, discriptorHandle);
	return discriptorIndex;
}

void VirtualTextureManager::unloadTextureHelper(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, StreamingManager& streamingManager)
{
	unsigned int descriptorIndex = std::numeric_limits<unsigned int>::max();
	unsigned int textureID;
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

		auto& resitencyInfo = texturesByID.data()[textureID];

		D3D12_PACKED_MIP_INFO packedMipInfo;
		graphicsEngine.graphicsDevice->GetResourceTiling(resitencyInfo.resource, nullptr, &packedMipInfo, nullptr, nullptr, 0u, nullptr);
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
			streamingManager.commandQueue()->UpdateTileMappings(resitencyInfo.resource, 1u, &resourceTileCoord, &tileRegion, nullptr, 1u, &rangeFlags, nullptr,
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
			tileRegion.Width = width;
			tileRegion.Height = height;
			tileRegion.Depth = 1u;

			D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;
			streamingManager.commandQueue()->UpdateTileMappings(resitencyInfo.resource, 1u, &resourceTileCoord, &tileRegion, nullptr, 1u, &rangeFlags, nullptr,
				nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		}
		
		delete[] resitencyInfo.pinnedHeapLocations;
		resitencyInfo.resource->Release();
		texturesByID.deallocate(textureID);
	}
}

void VirtualTextureManager::createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, RamToVramUploadRequest& vramRequest, const wchar_t* filename)
{
	D3D12Resource resource = createTexture(graphicsEngine.graphicsDevice, vramRequest);
#ifndef NDEBUG
	std::wstring name = L"virtual texture ";
	name += filename;
	resource->SetName(name.c_str());
#endif
	D3D12_PACKED_MIP_INFO packedMipInfo;
	D3D12_TILE_SHAPE tileShape;
	D3D12_SUBRESOURCE_TILING subresourceTiling;
	UINT numSubresourceTilings = 1u;
	graphicsEngine.graphicsDevice->GetResourceTiling(resource, nullptr, &packedMipInfo, &tileShape, &numSubresourceTilings, 0u, &subresourceTiling);
	unsigned int textureDescriptorIndex = createTextureDescriptor(graphicsEngine, resource, vramRequest);

	VirtualTextureInfo resitencyInfo;
	resitencyInfo.headerSize = vramRequest.file.getPosition();
	resitencyInfo.fileName = filename;
	resitencyInfo.widthInPages = subresourceTiling.WidthInTiles;
	resitencyInfo.heightInPages = subresourceTiling.HeightInTiles;
	resitencyInfo.pageWidthInTexels = tileShape.WidthInTexels;
	resitencyInfo.pageHeightInTexels = tileShape.HeightInTexels;
	resitencyInfo.numMipLevels = vramRequest.mipLevels;
	resitencyInfo.resource = resource;
	resitencyInfo.format = vramRequest.format;
	if (packedMipInfo.NumPackedMips == 0u)
	{
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
					pageProvider.pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsMax, resitencyInfo, commandQueue, graphicsEngine.graphicsDevice);
					resourceTileCoordsIndex = 0u;
				}
				resourceTileCoords[resourceTileCoordsIndex].X = x;
				resourceTileCoords[resourceTileCoordsIndex].Y = y;
				resourceTileCoords[resourceTileCoordsIndex].Z = 0u;
				resourceTileCoords[resourceTileCoordsIndex].Subresource = resitencyInfo.lowestPinnedMip;
				++resourceTileCoordsIndex;
			}
		}
		pageProvider.pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsIndex, resitencyInfo, commandQueue, graphicsEngine.graphicsDevice);
	}
	else
	{
		resitencyInfo.pinnedHeapLocations = new HeapLocation[packedMipInfo.NumPackedMips];
		resitencyInfo.lowestPinnedMip = vramRequest.mipLevels - packedMipInfo.NumPackedMips;
		pageProvider.pageAllocator.addPackedPages(resitencyInfo, packedMipInfo.NumTilesForPackedMips, commandQueue, graphicsEngine.graphicsDevice);
	}

	{
		auto width = vramRequest.width;
		auto height = vramRequest.height;
		size_t totalSize = 0u;
		for (auto i = 0u; i < resitencyInfo.lowestPinnedMip; ++i)
		{
			size_t numBytes, rowBytes, numRows;
			DDSFileLoader::getSurfaceInfo(width, height, vramRequest.format, numBytes, rowBytes, numRows);
			totalSize += numBytes;
			width >>= 1;
			if (width == 0u) width = 1u;
			height >>= 1;
			if (height == 0u) height = 1u;
		}
		vramRequest.file.setPosition(totalSize, ScopedFile::Position::current);
	}
	
	vramRequest.mostDetailedMip = resitencyInfo.lowestPinnedMip;
	vramRequest.currentMipLevel = resitencyInfo.lowestPinnedMip;
		
	std::lock_guard<decltype(mutex)> lock(mutex);
	auto& textureInfo = textures[filename];
	textureInfo.resource = resource.release();
	textureInfo.descriptorIndex = textureDescriptorIndex;

	auto requests = uploadRequests.find(filename);
	assert(requests != uploadRequests.end() && "A texture is loading with no requests for it");
	auto& request = requests->second[0];
	unsigned int textureID = texturesByID.allocate();
	textureInfo.textureID = textureID;
	texturesByID.data()[textureID] = std::move(resitencyInfo);
}

void VirtualTextureManager::textureUseSubresourceHelper(RamToVramUploadRequest& request, D3D12GraphicsEngine& graphicsEngine, StreamingManagerThreadLocal& streamingManager, void* const uploadBufferCpuAddressOfCurrentPos,
	ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
{
	ID3D12Device* graphicsDevice = graphicsEngine.graphicsDevice;
	const wchar_t* filename = reinterpret_cast<wchar_t*>(request.requester);

	ID3D12Resource* texture;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		texture = textures[filename].resource;
	}

	DDSFileLoader::loadSubresourceFromFile(graphicsDevice, request.width, request.height, request.depth, request.format, request.currentArrayIndex, request.currentMipLevel, request.mipLevels, request.file,
		uploadBufferCpuAddressOfCurrentPos, texture, streamingManager.currentCommandList, uploadResource, uploadResourceOffset);
}

void VirtualTextureManager::textureUploadedHelper(void* storedFilename, BaseExecutor* executor, SharedResources& sharedResources)
{
	const wchar_t* filename = reinterpret_cast<wchar_t*>(storedFilename);
	Texture* texture;
	std::vector<Request> requests;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		texture = &textures[filename];
		texture->loaded = true;

		auto request = uploadRequests.find(filename);
		assert(request != uploadRequests.end() && "A texture is loading with no requests for it");
		requests = std::move(request->second);
		uploadRequests.erase(request);
	}

	for (auto& request : requests)
	{
		request(executor, sharedResources, *texture);
	}
}

void VirtualTextureManager::TextureInfoAllocator::resize()
{
	if (freeListCapacity != 0u)
	{
		const auto oldFreeListCapacity = freeListCapacity;
		const auto newCap = oldFreeListCapacity + (oldFreeListCapacity >> 1u);

		delete[] freeList;
		freeList = new unsigned int[newCap];
		for (auto start = oldFreeListCapacity; start != newCap; ++start)
		{
			freeList[start - oldFreeListCapacity] = start;
		}

		VirtualTextureInfo* tempData = new VirtualTextureInfo[newCap];
		const auto end = mData + freeListCapacity;
		for (auto newStart = tempData, start = mData; start != end; ++start, ++newStart)
		{
			*newStart = *start;
		}
		delete[] mData;
		mData = tempData;
		freeListCapacity = newCap;
		freeListEnd = newCap - oldFreeListCapacity;
	}
	else
	{
		constexpr auto newCap = 8u;
		freeList = new unsigned int[newCap];
		for (auto i = 0u; i < newCap; ++i)
		{
			freeList[i] = i;
		}
		mData = new VirtualTextureInfo[newCap];
		freeListCapacity = newCap;
		freeListEnd = 8u;
	}
}