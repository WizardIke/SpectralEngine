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

void TextureManager::loadTextureUncached(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t * filename)
{
	File file = sharedResources.asynchronousFileManager.openFileForReading(sharedResources.ioCompletionQueue, filename);

	sharedResources.asynchronousFileManager.readFile(executor, sharedResources, filename, 0u, sizeof(DDSFileLoader::DdsHeaderDx12), file, reinterpret_cast<void*>(const_cast<wchar_t *>(filename)),
		[](void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* buffer, File file)
	{
		const DDSFileLoader::DdsHeaderDx12& header = *reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer);
		bool valid = DDSFileLoader::validateDdsHeader(header);
		if (!valid) throw false;
		RamToVramUploadRequest uploadRequest;
		uploadRequest.file = file;
		uploadRequest.useSubresource = textureUseSubresource;
		uploadRequest.requester = requester; //Actually the filename
		uploadRequest.resourceUploadedPointer = textureUploaded;
		uploadRequest.textureInfo.width = header.width;
		uploadRequest.textureInfo.height = header.height;
		uploadRequest.textureInfo.format = header.dxgiFormat;
		uploadRequest.mipLevels = header.mipMapCount;
		uploadRequest.currentArrayIndex = 0u;
		uploadRequest.currentMipLevel = 0u;
		uploadRequest.mostDetailedMip = 0u;
		uploadRequest.dimension = (D3D12_RESOURCE_DIMENSION)header.dimension;
		if (header.miscFlag & 0x4L)
		{
			uploadRequest.arraySize = header.arraySize * 6u; //The texture is a cubemap
		}
		else
		{
			uploadRequest.arraySize = header.arraySize;
		}
		uploadRequest.textureInfo.depth = header.depth;

		sharedResources.streamingManager.addUploadRequest(uploadRequest);
	});
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

ID3D12Resource* TextureManager::createOrLookupTexture(const HalfFinishedUploadRequest& useSubresourceRequest, SharedResources& sharedResources, const wchar_t* filename)
{
	ID3D12Resource* texture;
	TextureManager& textureManager = sharedResources.textureManager;
	auto& uploadRequest = *useSubresourceRequest.uploadRequest;
	if (useSubresourceRequest.currentArrayIndex == 0u && useSubresourceRequest.currentMipLevel == 0u)
	{
		D3D12_HEAP_PROPERTIES textureHeapProperties;
		textureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		textureHeapProperties.CreationNodeMask = 1u;
		textureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		textureHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		textureHeapProperties.VisibleNodeMask = 1u;

		D3D12_RESOURCE_DESC textureDesc;
		textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		textureDesc.DepthOrArraySize = std::max(uploadRequest.textureInfo.depth, uploadRequest.arraySize);
		textureDesc.Dimension = uploadRequest.dimension;
		textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		textureDesc.Format = uploadRequest.textureInfo.format;
		textureDesc.Height = uploadRequest.textureInfo.height;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.MipLevels = uploadRequest.mipLevels;
		textureDesc.SampleDesc.Count = 1u;
		textureDesc.SampleDesc.Quality = 0u;
		textureDesc.Width = uploadRequest.textureInfo.width;

		auto& graphicsEngine = sharedResources.graphicsEngine;
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
		srvDesc.Format = uploadRequest.textureInfo.format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (uploadRequest.dimension)
		{
		case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = uploadRequest.mipLevels;
			srvDesc.Texture3D.MostDetailedMip = 0u;
			srvDesc.Texture3D.ResourceMinLODClamp = 0u;
			break;
		case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (uploadRequest.arraySize > 1u)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.MipLevels = uploadRequest.mipLevels;
				srvDesc.Texture2DArray.MostDetailedMip = 0u;
				srvDesc.Texture2DArray.ResourceMinLODClamp = 0u;
				srvDesc.Texture2DArray.ArraySize = uploadRequest.arraySize;
				srvDesc.Texture2DArray.FirstArraySlice = 0u;
				srvDesc.Texture2DArray.PlaneSlice = 0u;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = uploadRequest.mipLevels;
				srvDesc.Texture2D.MostDetailedMip = 0u;
				srvDesc.Texture2D.ResourceMinLODClamp = 0u;
				srvDesc.Texture2D.PlaneSlice = 0u;
			}
			break;
		case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			if (uploadRequest.arraySize > 1u)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				srvDesc.Texture1DArray.MipLevels = uploadRequest.mipLevels;
				srvDesc.Texture1DArray.MostDetailedMip = 0u;
				srvDesc.Texture1DArray.ResourceMinLODClamp = 0u;
				srvDesc.Texture1DArray.ArraySize = uploadRequest.arraySize;
				srvDesc.Texture1DArray.FirstArraySlice = 0u;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MipLevels = uploadRequest.mipLevels;
				srvDesc.Texture1D.MostDetailedMip = 0u;
				srvDesc.Texture1D.ResourceMinLODClamp = 0u;
			}
			break;
		}
		graphicsEngine.graphicsDevice->CreateShaderResourceView(resource, &srvDesc, discriptorHandle);

		texture = resource;
		{
			std::lock_guard<decltype(textureManager.mutex)> lock(textureManager.mutex);
			Texture& texture = textureManager.textures[filename];
			texture.resource = std::move(resource);
			texture.descriptorIndex = discriptorIndex;
		}
	}
	else
	{
		std::lock_guard<decltype(textureManager.mutex)> lock(textureManager.mutex);
		texture = textureManager.textures[filename].resource;
	}

	return texture;
}

void TextureManager::textureUseSubresourceHelper(BaseExecutor* executor, SharedResources& sharedResources,  HalfFinishedUploadRequest& useSubresourceRequest)
{
	auto& uploadRequest = *useSubresourceRequest.uploadRequest;
	const wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);

	size_t subresouceWidth = uploadRequest.textureInfo.width;
	size_t subresourceHeight = uploadRequest.textureInfo.height;
	size_t subresourceDepth = uploadRequest.textureInfo.depth;
	size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);
	size_t currentMip = 0u;
	while (currentMip != useSubresourceRequest.currentMipLevel)
	{
		size_t numBytes, numRows, rowBytes;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.textureInfo.format, numBytes, rowBytes, numRows);
		fileOffset += numBytes * subresourceDepth;

		subresouceWidth >>= 1u;
		if (subresouceWidth == 0u) subresouceWidth = 1u;
		subresourceHeight >>= 1u;
		if (subresourceHeight == 0u) subresourceHeight = 1u;
		subresourceDepth >>= 1u;
		if (subresourceDepth == 0u) subresourceDepth = 1u;
	}
	
	size_t numBytes, numRows, rowBytes;
	DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.textureInfo.format, numBytes, rowBytes, numRows);
	size_t subresourceSize = numBytes * subresourceDepth;

	sharedResources.asynchronousFileManager.readFile(executor, sharedResources, filename, fileOffset, fileOffset + subresourceSize, uploadRequest.file, &useSubresourceRequest,
		[](void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* buffer, File file)
	{
		HalfFinishedUploadRequest& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
		auto& uploadRequest = *useSubresourceRequest.uploadRequest;
		const wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
		auto& streamingManager = executor->streamingManager;
		ID3D12Resource* destResource = createOrLookupTexture(useSubresourceRequest, sharedResources, filename);

		DDSFileLoader::copySubresourceToGpu(destResource, uploadRequest.uploadResource, useSubresourceRequest.uploadResourceOffset, uploadRequest.textureInfo.width, uploadRequest.textureInfo.height,
			uploadRequest.textureInfo.depth, useSubresourceRequest.currentMipLevel, uploadRequest.mipLevels, useSubresourceRequest.currentArrayIndex, uploadRequest.textureInfo.format, 
			(uint8_t*)useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos, buffer, streamingManager.copyCommandList());

		streamingManager.copyStarted(executor, useSubresourceRequest);
	});
}

void TextureManager::textureUseSubresource(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest)
{
	sharedResources.backgroundQueue.push(Job(&useSubresourceRequest, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
	{
		auto& textureManager = sharedResources.textureManager;
		textureManager.textureUseSubresourceHelper(executor, sharedResources, *reinterpret_cast<HalfFinishedUploadRequest*>(requester));
	}));
}