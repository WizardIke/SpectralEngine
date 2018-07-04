#include "TextureManager.h"
#include "DDSFileLoader.h"
#include "TextureIndexOutOfRangeException.h"
#include <d3d12.h>
#include "D3D12GraphicsEngine.h"

TextureManager::TextureManager() {}
TextureManager::~TextureManager() {}

void TextureManager::unloadTexture(const wchar_t* filename, D3D12GraphicsEngine& graphicsEngine)
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

ID3D12Resource* TextureManager::createOrLookupTexture(const HalfFinishedUploadRequest& useSubresourceRequest, TextureManager& textureManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t* filename)
{
	ID3D12Resource* texture;
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