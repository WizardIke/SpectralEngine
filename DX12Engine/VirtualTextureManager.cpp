#include "VirtualTextureManager.h"

void VirtualTextureManager::loadTextureUncachedHelper(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, const wchar_t * filename,
	void(*textureUseSubresource)(RamToVramUploadRequest* const request, BaseExecutor* executor1, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset), void(*textureUploaded)(void* storedFilename, BaseExecutor* exe))
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

void VirtualTextureManager::unloadTexture(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, unsigned int slot)
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

		auto& resitencyInfo = texturesByIDAndSlot[slot].data()[textureID];
		
		//deallocate pages in heap;
		D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[128];
		constexpr size_t resourceTileCoordsMax = sizeof(resourceTileCoords) / sizeof(resourceTileCoords[0u]);
		unsigned int resourceTileCoordsIndex = 0u;
		unsigned int index = 0u;
		for (auto i = 0u; i < resitencyInfo.heightInPages; ++i)
		{
			for (auto j = 0u; j < resitencyInfo.widthInPages; ++j)
			{
				auto mipLevel = resitencyInfo.mostDetailedMips[index];
				while(mipLevel < resitencyInfo.mostDetailedPackedMip)
				{
					//mip isn't packed
					if (i % mipLevel == 0 && j % mipLevel == 0)
					{
						if (resourceTileCoordsIndex == resourceTileCoordsMax)
						{
							pageProvider.removePages(resourceTileCoords, resourceTileCoordsMax, resitencyInfo);
							resourceTileCoordsIndex = 0u;
						}
						//add page at mip level to resourceTileCoords
						++resourceTileCoordsIndex;
					}
					++mipLevel;
				}
				++index;
			}
		}
		pageProvider.removePages(resourceTileCoords, resourceTileCoordsIndex, resitencyInfo);
		resourceTileCoords[0].X = 0u;
		resourceTileCoords[0].Y = 0u;
		resourceTileCoords[0].Z = 0u;
		resourceTileCoords[0].Subresource = resitencyInfo.mostDetailedPackedMip;
		D3D12_TILE_REGION_SIZE tileRegion;
		tileRegion.NumTiles = resitencyInfo.numTilesForPackedMips;
		tileRegion.UseBox = FALSE;
		pageProvider.removePackedPages(resourceTileCoords[0], tileRegion, resitencyInfo);

		delete[] resitencyInfo.heapLocations;
		delete[] resitencyInfo.mostDetailedMips;
		resitencyInfo.resource->Release();
		texturesByIDAndSlot[slot].deallocate(textureID);
	}
}

void VirtualTextureManager::createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, RamToVramUploadRequest& vramRequest, const wchar_t* filename)
{
	D3D12Resource resource = createTexture(graphicsEngine.graphicsDevice, vramRequest);
	D3D12_PACKED_MIP_INFO packedMipInfo;
	D3D12_TILE_SHAPE tileShape;
	D3D12_SUBRESOURCE_TILING subresourceTiling;
	uint32_t totalTiles;
	graphicsEngine.graphicsDevice->GetResourceTiling(resource, &totalTiles, &packedMipInfo, &tileShape, nullptr, 0u, &subresourceTiling);
	unsigned int textureDescriptorIndex = createTextureDescriptor(graphicsEngine, resource, vramRequest);

	TextureResitency resitencyInfo;
	resitencyInfo.widthInPages = subresourceTiling.WidthInTiles;
	resitencyInfo.heightInPages = subresourceTiling.HeightInTiles;
	resitencyInfo.widthInTexels = tileShape.WidthInTexels;
	resitencyInfo.heightInTexels = tileShape.HeightInTexels;
	resitencyInfo.numMipLevels = vramRequest.mipLevels;
	resitencyInfo.resource = resource;
	resitencyInfo.heapLocations = new HeapLocation[totalTiles];
	unsigned int numPages = resitencyInfo.widthInPages * resitencyInfo.heightInPages;
	resitencyInfo.mostDetailedMips = new unsigned char[numPages];
	unsigned char startingMip;
	if (packedMipInfo.NumPackedMips == 0u)
	{
		startingMip = vramRequest.mipLevels - 1u;
		resitencyInfo.mostDetailedPackedMip = vramRequest.mipLevels;

		D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[64];
		constexpr size_t resourceTileCoordsMax = sizeof(resourceTileCoords) / sizeof(resourceTileCoords[0u]);
		unsigned int resourceTileCoordsIndex = 0u;

		auto width = resitencyInfo.widthInPages >> startingMip;
		if (width == 0u) width = 1u;
		auto height = resitencyInfo.heightInPages >> startingMip;
		if (height == 0u) height = 1u;
		for (auto y = 0u; y < width; ++y)
		{
			for (auto x = 0u; x < width; ++x)
			{
				if (resourceTileCoordsIndex == resourceTileCoordsMax)
				{
					pageProvider.addPages(resourceTileCoords, resourceTileCoordsMax, resitencyInfo, commandQueue, graphicsEngine.graphicsDevice);
				}
				resourceTileCoords[resourceTileCoordsIndex].X = x;
				resourceTileCoords[resourceTileCoordsIndex].Y = y;
				resourceTileCoords[resourceTileCoordsIndex].Z = 1u;
				resourceTileCoords[resourceTileCoordsIndex].Subresource = startingMip;
			}
		}
		pageProvider.addPages(resourceTileCoords, resourceTileCoordsIndex, resitencyInfo, commandQueue, graphicsEngine.graphicsDevice);
	}
	else
	{
		unsigned char startingMip = vramRequest.mipLevels - packedMipInfo.NumPackedMips;
		resitencyInfo.mostDetailedPackedMip = startingMip;

		pageProvider.addPackedPages(resitencyInfo, commandQueue, graphicsEngine.graphicsDevice, &resitencyInfo.heapLocations[packedMipInfo.StartTileIndexInOverallResource]);
	}

	{
		auto width = resitencyInfo.widthInTexels;
		auto height = resitencyInfo.heightInTexels;
		size_t totalSize = 0u;
		for (auto i = 0u; i < startingMip; ++i)
		{
			size_t numBytes, rowBytes, numRows;
			DDSFileLoader::getSurfaceInfo(width, height, vramRequest.format, numBytes, rowBytes, numRows);
			totalSize += numBytes;
		}
		LONG highBits = (LONG)((totalSize & ~(size_t)(0xffffffff)) >> 32);
		SetFilePointer(vramRequest.file.file, totalSize & 0xffffffff, &highBits, FILE_CURRENT);
	}
	
	for (auto i = 0u; i < numPages; ++i)
	{
		resitencyInfo.mostDetailedMips[i] = startingMip;
	}
	vramRequest.mostDetailedMip = startingMip;
		
	std::lock_guard<decltype(mutex)> lock(mutex);
	auto& textureInfo = textures[filename];
	textureInfo.resource = resource.release();
	textureInfo.descriptorIndex = textureDescriptorIndex;

	auto requests = uploadRequests.find(filename);
	assert(requests != uploadRequests.end() && "A texture is loading with no requests for it");
	auto& request = requests->second[0];
	unsigned int slot = static_cast<unsigned int>(request.slot) - textureSlotsBaseBit;
	unsigned int textureID = texturesByIDAndSlot[slot].allocate();
	textureInfo.textureID = textureID;
	texturesByIDAndSlot[slot].data()[textureID] = std::move(resitencyInfo);
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

void VirtualTextureManager::textureUploadedHelper(void* storedFilename, BaseExecutor* executor)
{
	const wchar_t* filename = reinterpret_cast<wchar_t*>(storedFilename);
	unsigned int descriptorIndex;
	ID3D12Resource* resource;
	unsigned int textureID;
	std::vector<Request> requests;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		auto& texture = textures[filename];
		texture.loaded = true;

		resource = texture.resource;
		textureID = texture.textureID;
		descriptorIndex = texture.descriptorIndex;

		auto request = uploadRequests.find(filename);
		assert(request != uploadRequests.end() && "A texture is loading with no requests for it");
		requests = std::move(request->second);
		uploadRequests.erase(request);
	}

	for (auto& request : requests)
	{
		request(executor, descriptorIndex, textureID);
	}
}

void VirtualTextureManager::TextureResitencyAllocator::resize()
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

		TextureResitency* tempData = new TextureResitency[newCap];
		const auto end = mData + freeListCapacity;
		for (auto newStart = tempData, start = mData; start != end; ++start, ++newStart)
		{
			*newStart = *start;
		}
		delete[] mData;
		mData = tempData;
		freeListCapacity = newCap;
	}
	else
	{
		constexpr auto newCap = 8u;
		freeList = new unsigned int[newCap];
		for (auto i = 0u; i < newCap; ++i)
		{
			freeList[i] = i;
		}
		mData = new TextureResitency[newCap];
	}
}