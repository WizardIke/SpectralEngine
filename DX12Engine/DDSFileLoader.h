#pragma once

#include <d3d12.h>
#include "ScopedFile.h"

namespace DDSFileLoader
{
	void GetDDSTextureInfoFromFile(D3D12_RESOURCE_DESC& textureDesc, ScopedFile& textureFile, D3D12_SHADER_RESOURCE_VIEW_DESC& textureView, bool forceSRGB = false);

	void LoadDDSTextureFromFile(ID3D12Device* const Device, D3D12_RESOURCE_DESC& TextureDesc, const size_t FileByteSize, D3D12_SHADER_RESOURCE_VIEW_DESC& TextureSRVDesc, HANDLE const TextureFile,
		ID3D12Resource* const UploadTexture, ID3D12Resource* const DefaultTexture, ID3D12GraphicsCommandList* const CopyCommandList, D3D12_CPU_DESCRIPTOR_HANDLE const TextureDecriptorHandle, bool isCubeMap);

	void LoadDDsSubresourceFromFile(ID3D12Device* const Device, uint32_t textureWidth, uint32_t textureHeight, uint32_t textureDepth, DXGI_FORMAT format, ScopedFile TextureFile, void* const UploadBuffer, ID3D12Resource* const destResource,
		ID3D12GraphicsCommandList* const CopyCommandList, const uint32_t SubresourceIndex, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset);

	void getSurfaceInfo(size_t width, size_t height, DXGI_FORMAT fmt, size_t& outNumBytes, size_t& outRowBytes, size_t& outNumRows);
}