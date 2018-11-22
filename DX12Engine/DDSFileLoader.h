#pragma once

#include <d3d12.h>
#include "File.h"
#include <cstddef>

namespace DDSFileLoader
{
	struct TextureInfo
	{
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		uint32_t width;
		uint32_t height;
		uint32_t arraySize;
		uint32_t depth;
		uint32_t mipLevels;
		bool isCubeMap;
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

	struct DDS_HEADER
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
	void copySubresourceToGpu(ID3D12Resource* destResource, ID3D12Resource* uploadResource, unsigned long uploadBufferOffset, uint32_t width, uint32_t height, uint32_t depth, uint32_t currentMipLevel, uint32_t mipLevels,
		uint32_t currentArrayIndex, DXGI_FORMAT format, unsigned char* uploadBufferAddress, const unsigned char* sourceBuffer, ID3D12GraphicsCommandList* copyCommandList);
	void copySubresourceToGpuTiled(ID3D12Resource* destResource, ID3D12Resource* uploadResource, uint64_t uploadBufferOffset, uint32_t width, uint32_t height, uint32_t depth, uint32_t currentMipLevel, uint32_t mipLevels,
		uint32_t currentArrayIndex, DXGI_FORMAT format, unsigned char* uploadBufferAddress, const unsigned char* sourceBuffer, ID3D12GraphicsCommandList* copyCommandList);
	void copyResourceToGpu(ID3D12Resource* destResource, ID3D12Resource* uploadResource, unsigned long uploadBufferOffset, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t arraySize, DXGI_FORMAT format,
		unsigned char* uploadBufferAddress, const unsigned char* sourceBuffer, ID3D12GraphicsCommandList* copyCommandList);
	std::size_t alignedResourceSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t arraySize, DXGI_FORMAT format);
	std::size_t resourceSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t arraySize, DXGI_FORMAT format);
}