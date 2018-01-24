#include "TextureManager.h"
#include "DDSFileLoader.h"
#include "TextureIndexOutOfRangeException.h"
#include "SharedResources.h"
#include "BaseExecutor.h"
#include <d3d12.h>
#include "D3D12GraphicsEngine.h"

TextureManager::TextureManager() {}
TextureManager::~TextureManager() {}

void TextureManager::unloadTexture(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine)
{
	unsigned int descriptorIndex = std::numeric_limits<unsigned int>::max();
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto& texture = textures[filename];
		texture.numUsers -= 1u;
		if (texture.numUsers == 0u)
		{
			descriptorIndex = texture.descriptorIndex;
			textures.erase(filename);
		}
	}
	if (descriptorIndex != std::numeric_limits<unsigned int>::max())
	{
		graphicsEngine.descriptorAllocator.deallocate(descriptorIndex);
	}
}

void TextureManager::loadTextureUncachedHelper(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t * filename,
	void(*textureUseSubresource)(RamToVramUploadRequest* const request, BaseExecutor* executor1, SharedResources& sharedResources, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset))
{
	RamToVramUploadRequest& uploadRequest = streamingManager.getUploadRequest();
	uploadRequest.useSubresourcePointer = textureUseSubresource;
	uploadRequest.requester = reinterpret_cast<void*>(const_cast<wchar_t *>(filename));
	uploadRequest.resourceUploadedPointer = textureUploaded;

	uploadRequest.file.open(filename, ScopedFile::accessRight::genericRead, ScopedFile::shareMode::readMode, ScopedFile::creationMode::openExisting, nullptr);
	auto textureInfo = DDSFileLoader::getDDSTextureInfoFromFile(uploadRequest.file);

	uploadRequest.width = textureInfo.width;
	uploadRequest.height = textureInfo.height;
	uploadRequest.format = textureInfo.format;
	uploadRequest.mipLevels = textureInfo.mipLevels;
	uploadRequest.currentArrayIndex = 0u;
	uploadRequest.currentMipLevel = 0u;
	uploadRequest.mostDetailedMip = 0u;
	uploadRequest.dimension = textureInfo.dimension;

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
}

void TextureManager::textureUploaded(void* storedFilename, BaseExecutor* executor, SharedResources& sharedResources)
{
	const wchar_t* filename = reinterpret_cast<wchar_t*>(storedFilename);
	auto& textureManager = sharedResources.textureManager;

	unsigned int descriptorIndex;
	std::vector<Request> requests;
	{
		std::lock_guard<decltype(textureManager.mutex)> lock(textureManager.mutex);
		auto& texture = textureManager.textures[filename];
		texture.loaded = true;
		descriptorIndex = texture.descriptorIndex;

		auto requestsPos = textureManager.uploadRequests.find(filename);
		assert(requestsPos != textureManager.uploadRequests.end() && "A texture is loading with no requests for it");
		requests = std::move(requestsPos->second);
		textureManager.uploadRequests.erase(requestsPos);
	}

	for (auto& request : requests)
	{
		request(executor, sharedResources, descriptorIndex);
	}
}

void TextureManager::textureUseSubresource(RamToVramUploadRequest& request, D3D12GraphicsEngine& graphicsEngine, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
	uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager)
{
	ID3D12Resource* texture;
	const wchar_t* filename = reinterpret_cast<wchar_t*>(request.requester);
	if (request.currentArrayIndex == 0u && request.currentMipLevel == 0u)
	{
		D3D12_HEAP_PROPERTIES textureHeapProperties;
		textureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		textureHeapProperties.CreationNodeMask = 1u;
		textureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		textureHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		textureHeapProperties.VisibleNodeMask = 1u;

		D3D12_RESOURCE_DESC textureDesc;
		textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		textureDesc.DepthOrArraySize = std::max(request.depth, request.arraySize);
		textureDesc.Dimension = request.dimension;
		textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		textureDesc.Format = request.format;
		textureDesc.Height = request.height;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.MipLevels = request.mipLevels;
		textureDesc.SampleDesc.Count = 1u;
		textureDesc.SampleDesc.Quality = 0u;
		textureDesc.Width = request.width;

		D3D12Resource resource(graphicsEngine.graphicsDevice, textureHeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, textureDesc,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr);

#ifndef NDEBUG
		std::wstring name = L"texture ";
		name += filename;
		resource->SetName(name.c_str());
#endif // NDEBUG

		auto discriptorIndex = graphicsEngine.descriptorAllocator.allocate();
		D3D12_CPU_DESCRIPTOR_HANDLE discriptorHandle = graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
			discriptorIndex * graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = request.format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (request.dimension)
		{
		case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = request.mipLevels;
			srvDesc.Texture3D.MostDetailedMip = 0u;
			srvDesc.Texture3D.ResourceMinLODClamp = 0u;
			break;
		case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (request.arraySize > 1u)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.MipLevels = request.mipLevels;
				srvDesc.Texture2DArray.MostDetailedMip = 0u;
				srvDesc.Texture2DArray.ResourceMinLODClamp = 0u;
				srvDesc.Texture2DArray.ArraySize = request.arraySize;
				srvDesc.Texture2DArray.FirstArraySlice = 0u;
				srvDesc.Texture2DArray.PlaneSlice = 0u;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = request.mipLevels;
				srvDesc.Texture2D.MostDetailedMip = 0u;
				srvDesc.Texture2D.ResourceMinLODClamp = 0u;
				srvDesc.Texture2D.PlaneSlice = 0u;
			}
			break;
		case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			if (request.arraySize > 1u)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				srvDesc.Texture1DArray.MipLevels = request.mipLevels;
				srvDesc.Texture1DArray.MostDetailedMip = 0u;
				srvDesc.Texture1DArray.ResourceMinLODClamp = 0u;
				srvDesc.Texture1DArray.ArraySize = request.arraySize;
				srvDesc.Texture1DArray.FirstArraySlice = 0u;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MipLevels = request.mipLevels;
				srvDesc.Texture1D.MostDetailedMip = 0u;
				srvDesc.Texture1D.ResourceMinLODClamp = 0u;
			}
			break;
		}
		graphicsEngine.graphicsDevice->CreateShaderResourceView(resource, &srvDesc, discriptorHandle);

		texture = resource;
		{
			std::lock_guard<decltype(mutex)> lock(mutex);
			Texture& texture = textures[filename];
			texture.resource = std::move(resource);
			texture.descriptorIndex = discriptorIndex;
		}
	}
	else
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		texture = textures[filename].resource;
	}

	DDSFileLoader::loadSubresourceFromFile(graphicsEngine.graphicsDevice, request.width, request.height, request.depth, request.format, request.currentArrayIndex,
		request.currentMipLevel, request.mipLevels, request.file, uploadBufferCpuAddressOfCurrentPos, texture, streamingManager.currentCommandList,
		uploadResource, uploadResourceOffset);
}