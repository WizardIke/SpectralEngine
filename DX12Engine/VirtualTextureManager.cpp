#include "VirtualTextureManager.h"

VirtualTextureManager::VirtualTextureManager(PageProvider& pageProvider1, StreamingManager& streamingManager1, GraphicsEngine& graphicsEngine1, AsynchronousFileManager& asynchronousFileManager1) :
	pageProvider(pageProvider1),
	streamingManager(streamingManager1),
	graphicsEngine(graphicsEngine1),
	asynchronousFileManager(asynchronousFileManager1)
{}

void VirtualTextureManager::loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest,
	void(*useSubresource)(StreamingManager::StreamingRequest* request, void* tr),
	void callback(PageProvider::AllocateTextureRequest& request, void* tr),
	const DDSFileLoader::DdsHeaderDx12& header)
{
	bool valid = DDSFileLoader::validateDdsHeader(header);
	if (!valid) throw false;

	uploadRequest.streamResource = useSubresource;
	uploadRequest.dimension = (D3D12_RESOURCE_DIMENSION)header.dimension;
	Texture& texture = *uploadRequest.texture;
	texture.width = header.width;
	texture.height = header.height;
	texture.format = header.dxgiFormat;
	texture.numMipLevels = (unsigned int)header.mipMapCount;
	texture.file = uploadRequest.file;
	texture.filename = uploadRequest.filename;
	assert(header.depth == 1u);
	assert(header.arraySize == 1u);
	assert(header.dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D);
	createVirtualTexture(streamingManager.commandQueue(), callback, uploadRequest);
}

D3D12Resource VirtualTextureManager::createResource(ID3D12Device& graphicsDevice, const TextureStreamingRequest& request)
{
	D3D12_RESOURCE_DESC textureDesc;
	textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	textureDesc.DepthOrArraySize = 1u;
	textureDesc.Dimension = request.dimension;
	textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	textureDesc.Format = request.texture->format;
	textureDesc.Height = request.texture->height;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		
	textureDesc.MipLevels = static_cast<UINT16>(request.texture->numMipLevels);
	textureDesc.SampleDesc.Count = 1u;
	textureDesc.SampleDesc.Quality = 0u;
	textureDesc.Width = request.texture->width;

	return D3D12Resource(&graphicsDevice, textureDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
}

unsigned int VirtualTextureManager::createTextureDescriptor(GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request)
{
	auto discriptorIndex = graphicsEngine.descriptorAllocator.allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE discriptorHandle = graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		discriptorIndex * graphicsEngine.cbvAndSrvAndUavDescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv;
	srv.Format = request.texture->format;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (request.dimension)
	{
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
		srv.Texture3D.MipLevels = request.texture->numMipLevels;
		srv.Texture3D.MostDetailedMip = 0u;
		srv.Texture3D.ResourceMinLODClamp = 0u;
		break;
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = request.texture->numMipLevels;
		srv.Texture2D.MostDetailedMip = 0u;
		srv.Texture2D.ResourceMinLODClamp = 0u;
		srv.Texture2D.PlaneSlice = 0u;
		break;
	case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		srv.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
		srv.Texture1D.MipLevels = request.texture->numMipLevels;
		srv.Texture1D.MostDetailedMip = 0u;
		srv.Texture1D.ResourceMinLODClamp = 0u;
		break;
	}

	graphicsEngine.graphicsDevice->CreateShaderResourceView(texture, &srv, discriptorHandle);
	return discriptorIndex;
}

void VirtualTextureManager::unloadTexture(UnloadRequest& unloadRequest, void* tr)
{
	auto& texture = textures[unloadRequest.filename];
	--texture.numUsers;
	if (texture.numUsers == 0u)
	{
		const unsigned int descriptorIndex = texture.descriptorIndex;
		graphicsEngine.descriptorAllocator.deallocate(descriptorIndex);

		unloadRequest.textureInfo = &texture;
		unloadRequest.callback = [](PageProvider::UnloadRequest& request, void* tr)
		{
			auto& unloadRequest = static_cast<UnloadRequest&>(request);
			unloadRequest.textures->erase(unloadRequest.filename);

			static_cast<Message&>(unloadRequest).deleteReadRequest(unloadRequest, tr);
		};
		pageProvider.deleteTexture(unloadRequest);
	}
	else
	{
		static_cast<Message&>(unloadRequest).deleteReadRequest(unloadRequest, tr);
	}
}

void VirtualTextureManager::createVirtualTexture(ID3D12CommandQueue& commandQueue,
	void callback(PageProvider::AllocateTextureRequest& request, void* tr), TextureStreamingRequest& vramRequest)
{
	Texture& texture = *vramRequest.texture;
	texture.resource = createResource(*graphicsEngine.graphicsDevice, vramRequest);
#ifndef NDEBUG
	std::wstring name = L"virtual texture ";
	name += vramRequest.filename;
	vramRequest.texture->resource->SetName(name.c_str());
#endif
	D3D12_PACKED_MIP_INFO packedMipInfo;
	D3D12_TILE_SHAPE tileShape;
	D3D12_SUBRESOURCE_TILING subresourceTiling;
	UINT numSubresourceTilings = 1u;
	graphicsEngine.graphicsDevice->GetResourceTiling(texture.resource, nullptr, &packedMipInfo, &tileShape, &numSubresourceTilings, 0u, &subresourceTiling);
	unsigned int textureDescriptorIndex = createTextureDescriptor(graphicsEngine, texture.resource, vramRequest);

	//VirtualTextureInfo resitencyInfo;
	texture.widthInPages = subresourceTiling.WidthInTiles;
	texture.heightInPages = subresourceTiling.HeightInTiles;
	texture.descriptorIndex = textureDescriptorIndex;

	vramRequest.commandQueue = &commandQueue;
	vramRequest.graphicsDevice = graphicsEngine.graphicsDevice;
	static_cast<PageProvider::AllocateTextureRequest&>(vramRequest).textureInfo = vramRequest.texture;
	vramRequest.callback = callback;
	if (packedMipInfo.NumPackedMips == 0u)
	{
		//Pin lowest mip

		texture.lowestPinnedMip = texture.numMipLevels - 1u;
		auto width = texture.width >> texture.lowestPinnedMip;
		if (width == 0u) width = 1u;
		auto height = texture.height >> texture.lowestPinnedMip;
		if (height == 0u) height = 1u;
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(width, height, texture.format, numBytes, rowBytes, numRows);
		vramRequest.resourceSize = (unsigned long)numBytes;

		auto widthInPages = texture.widthInPages >> texture.lowestPinnedMip;
		if (widthInPages == 0u) widthInPages = 1u;
		auto heightInPages = texture.heightInPages >> texture.lowestPinnedMip;
		if (heightInPages == 0u) heightInPages = 1u;
		texture.pinnedPageCount = widthInPages * heightInPages;

		pageProvider.allocateTexturePinned(vramRequest);
	}
	else
	{
		texture.pinnedPageCount = packedMipInfo.NumPackedMips;
		texture.lowestPinnedMip = texture.numMipLevels - packedMipInfo.NumPackedMips;
		auto width = texture.width >> texture.lowestPinnedMip;
		if (width == 0u) width = 1u;
		auto height = texture.height >> texture.lowestPinnedMip;
		if (height == 0u) height = 1u;
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(width, height, texture.format, numBytes, rowBytes, numRows);
		vramRequest.resourceSize = (unsigned long)numBytes;

		pageProvider.allocateTexturePacked(vramRequest);
	}
}

void VirtualTextureManager::notifyTextureReady(TextureStreamingRequest* request, void* tr)
{
	auto& texture = textures[request->filename];
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
	std::size_t subresouceWidth = texture.width;
	std::size_t subresourceHeight = texture.height;
	std::size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

	for(std::size_t currentMip = 0u; currentMip != texture.lowestPinnedMip; ++currentMip)
	{
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, texture.format, numBytes, rowBytes, numRows);
		fileOffset += numBytes;

		subresouceWidth >>= 1u;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		subresourceHeight >>= 1u;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
	}

	std::size_t subresourceSize = 0u;
	for(std::size_t currentMip = texture.lowestPinnedMip; currentMip != texture.numMipLevels; ++currentMip)
	{
		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, texture.format, numBytes, rowBytes, numRows);
		subresourceSize += numBytes;

		subresouceWidth >>= 1u;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		subresourceHeight >>= 1u;
		if(subresourceHeight == 0u) subresourceHeight = 1u;
	}

	uploadRequest.start = fileOffset;
	uploadRequest.end = fileOffset + subresourceSize;
	uploadRequest.fileLoadedCallback = fileLoadedCallback;
}

void VirtualTextureManager::fileLoadedCallbackHelper(TextureStreamingRequest& uploadRequest, const unsigned char* buffer, StreamingManager::ThreadLocal& streamingManager,
	void(*copyFinished)(void* requester, void* tr))
{
	Texture& texture = *uploadRequest.texture;
	ID3D12Resource* destResource = texture.resource;

	unsigned long uploadResourceOffset = uploadRequest.uploadResourceOffset;
	unsigned char* uploadBufferCurrentCpuAddress = uploadRequest.uploadBufferCurrentCpuAddress;
	for(uint32_t i = texture.lowestPinnedMip;;)
	{
		uint32_t subresouceWidth = texture.width >> i;
		if(subresouceWidth == 0u) subresouceWidth = 1u;
		uint32_t subresourceHeight = texture.height >> i;
		if(subresourceHeight == 0u) subresourceHeight = 1u;

		DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, uploadResourceOffset, subresouceWidth, subresourceHeight,
			1u, i, texture.numMipLevels, 0u, texture.format, uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());

		++i;
		if(i == texture.numMipLevels) break;

		std::size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, texture.format, numBytes, rowBytes, numRows);
		std::size_t alignedSize = (rowBytes + (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) * numRows;
		uploadResourceOffset += (unsigned long)alignedSize;
		uploadBufferCurrentCpuAddress += alignedSize;
		buffer += numBytes;
	}
	streamingManager.addCopyCompletionEvent(&uploadRequest, copyFinished);
}