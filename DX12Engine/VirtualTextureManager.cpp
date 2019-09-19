#include "VirtualTextureManager.h"
#ifndef NDEBUG
#include <string>
#endif

VirtualTextureManager::VirtualTextureManager(PageProvider& pageProvider1, StreamingManager& streamingManager1, GraphicsEngine& graphicsEngine1, AsynchronousFileManager& asynchronousFileManager1) :
	pageProvider(pageProvider1),
	streamingManager(streamingManager1),
	graphicsEngine(graphicsEngine1),
	asynchronousFileManager(asynchronousFileManager1)
{}

void VirtualTextureManager::loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest,
	void(*useSubresource)(StreamingManager::StreamingRequest* request, void* tr),
	void (*callback)(PageProvider::AllocateTextureRequest& request, void* tr, VirtualTextureInfo& textureInfo),
	const DDSFileLoader::DdsHeaderDx12& header)
{
	bool valid = DDSFileLoader::validateDdsHeader(header);
	if (!valid) throw false;

	uploadRequest.streamResource = useSubresource;
	uploadRequest.dimension = (D3D12_RESOURCE_DIMENSION)header.dimension;
	uploadRequest.header = &header;
	assert(header.depth == 1u);
	assert(header.arraySize == 1u);
	assert(header.dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D);
	createVirtualTexture(graphicsEngine, pageProvider, callback, header, uploadRequest);
}

static D3D12Resource createResource(ID3D12Device& graphicsDevice, D3D12_RESOURCE_DIMENSION dimension, DXGI_FORMAT format, UINT64 width, UINT height, UINT16 numMipLevels)
{
	D3D12_RESOURCE_DESC textureDesc;
	textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	textureDesc.DepthOrArraySize = 1u;
	textureDesc.Dimension = dimension;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.Format = format;
	textureDesc.Height = height;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		
	textureDesc.MipLevels = numMipLevels;
	textureDesc.SampleDesc.Count = 1u;
	textureDesc.SampleDesc.Quality = 0u;
	textureDesc.Width = width;

	return D3D12Resource(&graphicsDevice, textureDesc, D3D12_RESOURCE_STATE_COMMON);
}

static unsigned int createTextureDescriptor(GraphicsEngine& graphicsEngine, ID3D12Resource* texture, DXGI_FORMAT format, D3D12_RESOURCE_DIMENSION dimension, UINT numMipLevels)
{
	auto discriptorIndex = graphicsEngine.descriptorAllocator.allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE discriptorHandle = graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		discriptorIndex * graphicsEngine.cbvAndSrvAndUavDescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv;
	srv.Format = format;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (dimension)
	{
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
		srv.Texture3D.MipLevels = numMipLevels;
		srv.Texture3D.MostDetailedMip = 0u;
		srv.Texture3D.ResourceMinLODClamp = 0u;
		break;
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = numMipLevels;
		srv.Texture2D.MostDetailedMip = 0u;
		srv.Texture2D.ResourceMinLODClamp = 0u;
		srv.Texture2D.PlaneSlice = 0u;
		break;
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
		srv.Texture1D.MipLevels = numMipLevels;
		srv.Texture1D.MostDetailedMip = 0u;
		srv.Texture1D.ResourceMinLODClamp = 0u;
		break;
	}

	graphicsEngine.graphicsDevice->CreateShaderResourceView(texture, &srv, discriptorHandle);
	return discriptorIndex;
}

void VirtualTextureManager::unloadTexture(UnloadRequest& unloadRequest, void* tr)
{
	auto texturePtr = textures.find({ unloadRequest.start, unloadRequest.end });
	auto& texture = texturePtr->second;
	--texture.numUsers;
	if (texture.numUsers == 0u)
	{
		const unsigned int descriptorIndex = texture.descriptorIndex;
		graphicsEngine.descriptorAllocator.deallocate(descriptorIndex);

		unloadRequest.textureInfo = texture.info;
		textures.erase(texturePtr);

		unloadRequest.callback = [](PageProvider::UnloadRequest& request, void* tr)
		{
			auto& unloadRequest = static_cast<UnloadRequest&>(request);

			static_cast<Message&>(unloadRequest).deleteReadRequest(unloadRequest, tr);
		};
		pageProvider.deleteTexture(unloadRequest);
	}
	else
	{
		static_cast<Message&>(unloadRequest).deleteReadRequest(unloadRequest, tr);
	}
}

void VirtualTextureManager::createVirtualTexture(GraphicsEngine& graphicsEngine, PageProvider& pageProvider, void (*callback)(PageProvider::AllocateTextureRequest& request, void* tr, VirtualTextureInfo& textureInfo),
	const DDSFileLoader::DdsHeaderDx12& header, TextureStreamingRequest& uploadRequest)
{
	auto resource = createResource(*graphicsEngine.graphicsDevice, static_cast<D3D12_RESOURCE_DIMENSION>(header.dimension), header.dxgiFormat,
		header.width, header.height, static_cast<UINT16>(header.mipMapCount));
#ifndef NDEBUG
	std::wstring name = L"virtual texture {";
	name += std::to_wstring(uploadRequest.resourceLocation.start);
	name += L", ";
	name += std::to_wstring(uploadRequest.resourceLocation.end);
	name += L"}";
	resource->SetName(name.c_str());
#endif

	D3D12_PACKED_MIP_INFO packedMipInfo;
	D3D12_TILE_SHAPE tileShape;
	D3D12_SUBRESOURCE_TILING subresourceTiling;
	UINT numSubresourceTilings = 1u;
	graphicsEngine.graphicsDevice->GetResourceTiling(resource, nullptr, &packedMipInfo, &tileShape, &numSubresourceTilings, 0u, &subresourceTiling);

	auto& allocateTextureRequest = static_cast<PageProvider::AllocateTextureRequest&>(uploadRequest);
	allocateTextureRequest.resource = std::move(resource);
	allocateTextureRequest.widthInPages = subresourceTiling.WidthInTiles;
	allocateTextureRequest.heightInPages = subresourceTiling.HeightInTiles;
	allocateTextureRequest.callback = callback;

	if (packedMipInfo.NumPackedMips == 0u)
	{
		//Pin lowest mip
		allocateTextureRequest.lowestPinnedMip = header.mipMapCount - 1u;
		pageProvider.allocateTexturePinned(allocateTextureRequest);
	}
	else
	{
		allocateTextureRequest.lowestPinnedMip = header.mipMapCount - packedMipInfo.NumPackedMips;
		allocateTextureRequest.pinnedPageCount = packedMipInfo.NumPackedMips;

		pageProvider.allocateTexturePacked(allocateTextureRequest);
	}
}

void VirtualTextureManager::textureCreatedHealper(TextureStreamingRequest& uploadRequest, VirtualTextureInfo& textureInfo)
{
	auto& graphicsEngine = uploadRequest.virtualTextureManager->graphicsEngine;
	auto& texture = *uploadRequest.texture;
	auto& header = *uploadRequest.header;
	texture.info = &textureInfo;
	textureInfo.width = header.width;
	textureInfo.height = header.height;
	textureInfo.format = header.dxgiFormat;
	textureInfo.numMipLevels = static_cast<unsigned int>(header.mipMapCount);
	textureInfo.resourceStart = uploadRequest.resourceLocation.start;

	texture.descriptorIndex = createTextureDescriptor(graphicsEngine, textureInfo.resource, textureInfo.format, uploadRequest.dimension, textureInfo.numMipLevels);

	auto width = textureInfo.width >> textureInfo.lowestPinnedMip;
	if (width == 0u) width = 1u;
	auto height = textureInfo.height >> textureInfo.lowestPinnedMip;
	if (height == 0u) height = 1u;
	std::size_t numBytes, numRows, rowBytes;
	DDSFileLoader::surfaceInfo(width, height, textureInfo.format, numBytes, rowBytes, numRows);
	uploadRequest.resourceSize = (unsigned long)numBytes;
}

void VirtualTextureManager::notifyTextureReady(TextureStreamingRequest* request, void* tr)
{
	auto& texture = textures[request->resourceLocation];
	texture.lastRequest = nullptr;
	do
	{
		auto old = request;
		request = request->nextTextureRequest; //Need to do this now as old could be deleted by the next line
		old->textureLoaded(*old, tr, texture);
	} while(request != nullptr);
}

void VirtualTextureManager::textureUseResourceHelper(TextureStreamingRequest& uploadRequest,
	void(*fileLoadedCallback)(AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* buffer))
{
	Texture& texture = *uploadRequest.texture;
	auto& textureInfo = *texture.info;
	std::size_t subresouceWidth = textureInfo.width;
	std::size_t subresourceHeight = textureInfo.height;
	std::size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

	const auto format = textureInfo.format;
	const auto lowestPinnedMip = textureInfo.lowestPinnedMip;
	for(std::size_t currentMip = 0u; currentMip != lowestPinnedMip; ++currentMip)
	{
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, format, numBytes, rowBytes, numRows);
		fileOffset += numBytes;

		subresouceWidth >>= 1u;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		subresourceHeight >>= 1u;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
	}

	std::size_t subresourceSize = 0u;
	const auto numMipLevels = textureInfo.numMipLevels;
	for(std::size_t currentMip = lowestPinnedMip; currentMip != numMipLevels; ++currentMip)
	{
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, format, numBytes, rowBytes, numRows);
		subresourceSize += numBytes;

		subresouceWidth >>= 1u;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		subresourceHeight >>= 1u;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
	}

	uploadRequest.start = uploadRequest.resourceLocation.start + fileOffset;
	uploadRequest.end = uploadRequest.start + subresourceSize;
	uploadRequest.fileLoadedCallback = fileLoadedCallback;
}

void VirtualTextureManager::fileLoadedCallbackHelper(TextureStreamingRequest& uploadRequest, const unsigned char* buffer, StreamingManager::ThreadLocal& streamingManager,
	void(*copyFinished)(void* requester, void* tr))
{
	Texture& texture = *uploadRequest.texture;
	auto& textureInfo = *texture.info;
	ID3D12Resource* destResource = textureInfo.resource;

	unsigned long uploadResourceOffset = uploadRequest.uploadResourceOffset;
	unsigned char* uploadBufferCurrentCpuAddress = uploadRequest.uploadBufferCurrentCpuAddress;
	for(uint32_t i = textureInfo.lowestPinnedMip;;)
	{
		uint32_t subresouceWidth = textureInfo.width >> i;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		uint32_t subresourceHeight = textureInfo.height >> i;
		if(subresourceHeight == 0u) subresourceHeight = 1u;

		DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, uploadResourceOffset, subresouceWidth, subresourceHeight,
			1u, i, textureInfo.numMipLevels, 0u, textureInfo.format, uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());

		++i;
		if(i == textureInfo.numMipLevels) break;

		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, textureInfo.format, numBytes, rowBytes, numRows);
		std::size_t alignedSize = (rowBytes + (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) * numRows;
		uploadResourceOffset += (unsigned long)alignedSize;
		uploadBufferCurrentCpuAddress += alignedSize;
		buffer += numBytes;
	}
	streamingManager.addCopyCompletionEvent(&uploadRequest, copyFinished);
}