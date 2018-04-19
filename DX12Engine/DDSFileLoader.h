#pragma once

#include <d3d12.h>
#include "ScopedFile.h"

namespace DDSFileLoader
{
	struct TextureInfo
	{
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		uint32_t width;
		uint32_t height;
		uint32_t depthOrArraySize;
		uint32_t mipLevels;
	};

	struct DDS_PIXELFORMAT
	{
		uint32_t    size;
		uint32_t    flags;
		uint32_t    fourCC;
		uint32_t    RGBBitCount;
		uint32_t    RBitMask;
		uint32_t    GBitMask;
		uint32_t    BBitMask;
		uint32_t    ABitMask;
	};

	struct DdsHeaderDx12
	{
		uint32_t ddsMagicNumber;
		uint32_t        size;
		uint32_t        flags;
		uint32_t        height;
		uint32_t        width;
		uint32_t        pitchOrLinearSize;
		uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
		uint32_t        mipMapCount;
		uint32_t        reserved1[11];
		DDS_PIXELFORMAT ddspf;
		uint32_t        caps;
		uint32_t        caps2;
		uint32_t        caps3;
		uint32_t        caps4;
		uint32_t        reserved2;

		//We only use files with the DXT10 file header
		DXGI_FORMAT     dxgiFormat;
		uint32_t        dimension;
		uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
		uint32_t        arraySize;
		uint32_t        miscFlags2;
	};

	void surfaceInfo(size_t width, size_t height, DXGI_FORMAT fmt, size_t& outNumBytes, size_t& outRowBytes, size_t& outNumRows);
	size_t bitsPerPixel(DXGI_FORMAT fmt);
	void tileWidthAndHeightAndTileWidthInBytes(DXGI_FORMAT format, uint32_t& width, uint32_t& height, uint32_t& tileWidthBytes);
	bool validateDdsHeader(const DdsHeaderDx12& header);
	void copySubresourceToGpu(ID3D12Resource* destResource, ID3D12Resource* uploadResource, uint64_t uploadBufferOffset, uint32_t width, uint32_t height, uint32_t depth, uint32_t currentMipLevel, uint32_t mipLevels,
		uint32_t currentArrayIndex, DXGI_FORMAT format, uint8_t* uploadBufferAddress, const uint8_t* sourceBuffer, ID3D12GraphicsCommandList* copyCommandList);
	void copySubresourceToGpuTiled(ID3D12Resource* destResource, ID3D12Resource* uploadResource, uint64_t uploadBufferOffset, uint32_t width, uint32_t height, uint32_t depth, uint32_t currentMipLevel, uint32_t mipLevels,
		uint32_t currentArrayIndex, DXGI_FORMAT format, uint8_t* uploadBufferAddress, const uint8_t* sourceBuffer, ID3D12GraphicsCommandList* copyCommandList);


	TextureInfo getDDSTextureInfoFromFile(File& textureFile);
	void getDDSTextureInfoFromFile(D3D12_RESOURCE_DESC& textureDesc, File& textureFile, D3D12_SHADER_RESOURCE_VIEW_DESC& textureView, bool forceSRGB = false);
	void LoadDDSTextureFromFile(ID3D12Device* const Device, D3D12_RESOURCE_DESC& TextureDesc, const size_t FileByteSize, D3D12_SHADER_RESOURCE_VIEW_DESC& TextureSRVDesc, HANDLE const TextureFile,
		ID3D12Resource* const UploadTexture, ID3D12Resource* const DefaultTexture, ID3D12GraphicsCommandList* const CopyCommandList, D3D12_CPU_DESCRIPTOR_HANDLE const TextureDecriptorHandle, bool isCubeMap);
	void loadSubresourceFromFile(ID3D12Device* const graphicsDevice, uint64_t textureWidth, uint32_t textureHeight, uint32_t textureDepth, DXGI_FORMAT format, uint16_t arrayIndex, uint16_t mipLevel, uint16_t mipLevels,
		File TextureFile, void* const UploadBuffer, ID3D12Resource* const destResource, ID3D12GraphicsCommandList* const CopyCommandList, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset);
}