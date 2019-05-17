#include "VirtualTextureManager.h"

VirtualTextureManager::VirtualTextureManager() {}

void VirtualTextureManager::loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine,
	void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
	const DDSFileLoader::DdsHeaderDx12& header)
{
	bool valid = DDSFileLoader::validateDdsHeader(header);
	if (!valid) throw false;

	uploadRequest.streamResource = useSubresource;
	uploadRequest.width = header.width;
	uploadRequest.height = header.height;
	uploadRequest.format = header.dxgiFormat;
	uploadRequest.mipLevels = (uint16_t)header.mipMapCount;
	uploadRequest.dimension = (D3D12_RESOURCE_DIMENSION)header.dimension;
	uploadRequest.depth = (uint16_t)header.depth;
	assert(header.depth == 1u);
	assert(header.arraySize == 1u);
	createTextureWithResitencyInfo(graphicsEngine, streamingManager.commandQueue(), uploadRequest);

	auto width = header.width >> uploadRequest.lowestPinnedMip;
	if(width == 0u) width = 1u;
	auto height = header.height >> uploadRequest.lowestPinnedMip;
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

unsigned int VirtualTextureManager::createTextureDescriptor(GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request)
{
	auto discriptorIndex = graphicsEngine.descriptorAllocator.allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE discriptorHandle = graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		discriptorIndex * graphicsEngine.cbvAndSrvAndUavDescriptorSize;

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

void VirtualTextureManager::unloadTexture(const wchar_t * filename, GraphicsEngine& graphicsEngine)
{
	//unsigned int descriptorIndex = std::numeric_limits<unsigned int>::max();
	//unsigned int textureID = 255;
	{
		auto& texture = textures[filename];
		texture.numUsers -= 1u;
		if (texture.numUsers == 0u)
		{
			unsigned int descriptorIndex = texture.descriptorIndex;
			unsigned int textureID = texture.textureID;
			textures.erase(filename);

			graphicsEngine.descriptorAllocator.deallocate(descriptorIndex);
			assert(textureID != 255);
			auto& resitencyInfo = texturesByID[textureID];

			pageProvider.pageAllocator.removePinnedPages(resitencyInfo.pinnedHeapLocations, resitencyInfo.pinnedPageCount);

			delete[] resitencyInfo.pinnedHeapLocations;
			resitencyInfo.resource->Release();
			resitencyInfo.~VirtualTextureInfo();
			texturesByID.deallocate(textureID);
		}
	}
}

void VirtualTextureManager::createTextureWithResitencyInfo(GraphicsEngine& graphicsEngine, ID3D12CommandQueue& commandQueue, TextureStreamingRequest& vramRequest)
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

	//VirtualTextureInfo resitencyInfo;
	vramRequest.widthInPages = subresourceTiling.WidthInTiles;
	vramRequest.heightInPages = subresourceTiling.HeightInTiles;
	vramRequest.resource = resource.steal();
	vramRequest.descriptorIndex = textureDescriptorIndex;
	if (packedMipInfo.NumPackedMips == 0u)
	{
		//Pin lowest mip

		vramRequest.lowestPinnedMip = vramRequest.mipLevels - 1u;

		D3D12_TILED_RESOURCE_COORDINATE resourceTileCoords[64];
		constexpr size_t resourceTileCoordsMax = sizeof(resourceTileCoords) / sizeof(resourceTileCoords[0u]);
		unsigned int resourceTileCoordsIndex = 0u;

		auto width = vramRequest.widthInPages >> vramRequest.lowestPinnedMip;
		if (width == 0u) width = 1u;
		auto height = vramRequest.heightInPages >> vramRequest.lowestPinnedMip;
		if (height == 0u) height = 1u;
		vramRequest.pinnedPageCount = width * height;
		vramRequest.pinnedHeapLocations = new GpuHeapLocation[vramRequest.pinnedPageCount];
		for (auto y = 0u; y < height; ++y)
		{
			for (auto x = 0u; x < width; ++x)
			{
				if (resourceTileCoordsIndex == resourceTileCoordsMax)
				{
					pageProvider.pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsMax, vramRequest.resource, vramRequest.pinnedHeapLocations, &commandQueue, graphicsEngine.graphicsDevice);
					resourceTileCoordsIndex = 0u;
				}
				resourceTileCoords[resourceTileCoordsIndex].X = x;
				resourceTileCoords[resourceTileCoordsIndex].Y = y;
				resourceTileCoords[resourceTileCoordsIndex].Z = 0u;
				resourceTileCoords[resourceTileCoordsIndex].Subresource = vramRequest.lowestPinnedMip;
				++resourceTileCoordsIndex;
			}
		}
		pageProvider.pageAllocator.addPinnedPages(resourceTileCoords, resourceTileCoordsIndex, vramRequest.resource, vramRequest.pinnedHeapLocations, &commandQueue, graphicsEngine.graphicsDevice);
	}
	else
	{
		vramRequest.pinnedPageCount = packedMipInfo.NumPackedMips;
		vramRequest.pinnedHeapLocations = new GpuHeapLocation[packedMipInfo.NumPackedMips];
		vramRequest.lowestPinnedMip = vramRequest.mipLevels - packedMipInfo.NumPackedMips;
		pageProvider.pageAllocator.addPinnedPages(vramRequest.resource, vramRequest.pinnedHeapLocations, vramRequest.lowestPinnedMip, packedMipInfo.NumTilesForPackedMips, &commandQueue, graphicsEngine.graphicsDevice);
	}
}

static void addResitencyInfo(VirtualTextureInfoByID& texturesByID, VirtualTextureManager::Texture& textureInfo, VirtualTextureManager::TextureStreamingRequest& request)
{
	textureInfo.resource = request.resource;
	textureInfo.descriptorIndex = request.descriptorIndex;
	unsigned int textureID = texturesByID.allocate();
	textureInfo.textureID = textureID;

	auto& resitencyInfo = texturesByID[textureID];
	new(&resitencyInfo) VirtualTextureInfo{};
	resitencyInfo.resource = request.resource;
	resitencyInfo.widthInPages = request.widthInPages;
	resitencyInfo.heightInPages = request.heightInPages;
	resitencyInfo.numMipLevels = request.mipLevels;
	resitencyInfo.lowestPinnedMip = request.lowestPinnedMip;
	resitencyInfo.format = request.format;
	resitencyInfo.width = request.width;
	resitencyInfo.height = request.height;
	resitencyInfo.file = request.file;
	resitencyInfo.filename = request.filename;
	resitencyInfo.pinnedHeapLocations = request.pinnedHeapLocations;
	resitencyInfo.pinnedPageCount = request.pinnedPageCount;
}

void VirtualTextureManager::notifyTextureReady(TextureStreamingRequest* request, void* tr, void* gr)
{
	auto& texture = textures[request->filename];
	texture.lastRequest = nullptr;
	addResitencyInfo(texturesByID, texture, *request);
	do
	{
		auto old = request;
		request = request->nextTextureRequest; //Need to do this now as old could be deleted by the next line
		old->textureLoaded(*old, tr, gr, texture);
	} while(request != nullptr);
}

void VirtualTextureManager::textureUseResourceHelper(TextureStreamingRequest& uploadRequest, void(*fileLoadedCallback)(AsynchronousFileManager::ReadRequest& request, void* tr, void*, const unsigned char* buffer))
{
	std::size_t subresouceWidth = uploadRequest.width;
	std::size_t subresourceHeight = uploadRequest.height;
	std::size_t subresourceDepth = uploadRequest.depth;
	std::size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

	for(std::size_t currentMip = 0u; currentMip != uploadRequest.lowestPinnedMip; ++currentMip)
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
	for(std::size_t currentMip = uploadRequest.lowestPinnedMip; currentMip != uploadRequest.mipLevels; ++currentMip)
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
	void(*copyFinished)(void* requester, void* tr, void* gr))
{
	ID3D12Resource* destResource = uploadRequest.resource;

	unsigned long uploadResourceOffset = uploadRequest.uploadResourceOffset;
	unsigned char* uploadBufferCurrentCpuAddress = uploadRequest.uploadBufferCurrentCpuAddress;
	for(uint32_t i = uploadRequest.lowestPinnedMip;;)
	{
		uint32_t subresouceWidth = uploadRequest.width >> i;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		uint32_t subresourceHeight = uploadRequest.height >> i;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
		uint32_t subresourceDepth = uploadRequest.depth >> i;
		if(subresourceDepth == 0u) subresourceDepth = 1u;

		DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, uploadResourceOffset, subresouceWidth, subresourceHeight,
			subresourceDepth, i, uploadRequest.mipLevels, 0u, uploadRequest.format, uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());

		++i;
		if(i == uploadRequest.mipLevels) break;

		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.format, numBytes, rowBytes, numRows);
		std::size_t alignedSize = (rowBytes + (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) * numRows * subresourceDepth;
		uploadResourceOffset += (unsigned long)alignedSize;
		uploadBufferCurrentCpuAddress += alignedSize;
		buffer += numBytes;
	}
	streamingManager.addCopyCompletionEvent(&uploadRequest, copyFinished);
}