#include "DDSFileLoader.h"

#include <cassert>
#include <algorithm>
#include <d3d12.h>
#include "d3dx12.h"
#include <memory>
#include "IOException.h"
#include "HresultException.h"
#include "FormatNotSupportedException.h"
#include "EndOfFileException.h"
#include "InvalidHeaderInfoException.h"
#include "SecurityException.h"
#include "IndexOutOfRangeException.h"

namespace DDSFileLoader
{
	constexpr static uint32_t DDS_MAGIC = 0x20534444;
	constexpr static uint32_t DDS_FOURCC = 0x00000004;  // DDPF_FOURCC
	constexpr static uint32_t DDS_RGB   =      0x00000040;  // DDPF_RGB
	constexpr static uint32_t DDS_LUMINANCE =  0x00020000;  // DDPF_LUMINANCE
	constexpr static uint32_t DDS_ALPHA    =   0x00000002;  // DDPF_ALPHA

	constexpr static uint32_t DDS_HEADER_FLAGS_VOLUME = 0x00800000;  // DDSD_DEPTH

	constexpr static uint32_t DDS_HEIGHT = 0x00000002; // DDSD_HEIGHT
	constexpr static uint32_t DDS_WIDTH = 0x00000004; // DDSD_WIDTH

	constexpr static uint32_t DDS_CUBEMAP_POSITIVEX = 0x00000600; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
	constexpr static uint32_t DDS_CUBEMAP_NEGATIVEX = 0x00000a00; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
	constexpr static uint32_t DDS_CUBEMAP_POSITIVEY = 0x00001200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
	constexpr static uint32_t DDS_CUBEMAP_NEGATIVEY = 0x00002200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
	constexpr static uint32_t DDS_CUBEMAP_POSITIVEZ = 0x00004200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
	constexpr static uint32_t DDS_CUBEMAP_NEGATIVEZ = 0x00008200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

	constexpr static uint32_t DDS_CUBEMAP_ALLFACES = (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX | DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY | DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ);

	constexpr static uint32_t DDS_CUBEMAP = 0x00000200; // DDSCAPS2_CUBEMAP

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

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

	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT     dxgiFormat;
		uint32_t        dimension;
		uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
		uint32_t        arraySize;
		uint32_t        miscFlags2;
	};

	static size_t bitsPerPixel(DXGI_FORMAT fmt)
	{
		switch (fmt)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_Y416:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:
		case DXGI_FORMAT_YUY2:
			return 32;

		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
			return 24;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_A8P8:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return 16;

		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_420_OPAQUE:
		case DXGI_FORMAT_NV11:
			return 12;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_AI44:
		case DXGI_FORMAT_IA44:
		case DXGI_FORMAT_P8:
			return 8;

		case DXGI_FORMAT_R1_UNORM:
			return 1;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;

		default:
			return 0;
		}
	}



	static DXGI_FORMAT getDXGIFormat(const DDS_PIXELFORMAT& ddpf)
	{
#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )
		if (ddpf.flags & DDS_RGB)
		{
			// Note that sRGB formats are written using the "DX10" extended header

			switch (ddpf.RGBBitCount)
			{
			case 32:
				if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
				{
					return DXGI_FORMAT_R8G8B8A8_UNORM;
				}

				if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
				{
					return DXGI_FORMAT_B8G8R8A8_UNORM;
				}

				if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
				{
					return DXGI_FORMAT_B8G8R8X8_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

				// Note that many common DDS reader/writers (including D3DX) swap the
				// the RED/BLUE masks for 10:10:10:2 formats. We assumme
				// below that the 'backwards' header mask is being used since it is most
				// likely written by D3DX. The more robust solution is to use the 'DX10'
				// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

				// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
				if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
				{
					return DXGI_FORMAT_R10G10B10A2_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

				if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R16G16_UNORM;
				}

				if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
				{
					// Only 32-bit color channel format in D3D9 was R32F
					return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
				}
				break;

			case 24:
				// No 24bpp DXGI formats aka D3DFMT_R8G8B8
				break;

			case 16:
				if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
				{
					return DXGI_FORMAT_B5G5R5A1_UNORM;
				}
				if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
				{
					return DXGI_FORMAT_B5G6R5_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

				if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
				{
					return DXGI_FORMAT_B4G4R4A4_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

				// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
				break;
			}
		}
		else if (ddpf.flags & DDS_LUMINANCE)
		{
			if (8 == ddpf.RGBBitCount)
			{
				if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
			}

			if (16 == ddpf.RGBBitCount)
			{
				if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
				}
				if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
				{
					return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}
			}
		}
		else if (ddpf.flags & DDS_ALPHA)
		{
			if (8 == ddpf.RGBBitCount)
			{
				return DXGI_FORMAT_A8_UNORM;
			}
		}
		else if (ddpf.flags & DDS_FOURCC)
		{
			if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC1_UNORM;
			}
			if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			// While pre-mulitplied alpha isn't directly supported by the DXGI formats,
			// they are basically the same as these BC formats so they can be mapped
			if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_SNORM;
			}

			if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_SNORM;
			}

			// BC6H and BC7 are written using the "DX10" extended header

			if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC)
			{
				return DXGI_FORMAT_R8G8_B8G8_UNORM;
			}
			if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC)
			{
				return DXGI_FORMAT_G8R8_G8B8_UNORM;
			}

			if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
			{
				return DXGI_FORMAT_YUY2;
			}

			// Check for D3DFORMAT enums being set here
			switch (ddpf.fourCC)
			{
			case 36: // D3DFMT_A16B16G16R16
				return DXGI_FORMAT_R16G16B16A16_UNORM;

			case 110: // D3DFMT_Q16W16V16U16
				return DXGI_FORMAT_R16G16B16A16_SNORM;

			case 111: // D3DFMT_R16F
				return DXGI_FORMAT_R16_FLOAT;

			case 112: // D3DFMT_G16R16F
				return DXGI_FORMAT_R16G16_FLOAT;

			case 113: // D3DFMT_A16B16G16R16F
				return DXGI_FORMAT_R16G16B16A16_FLOAT;

			case 114: // D3DFMT_R32F
				return DXGI_FORMAT_R32_FLOAT;

			case 115: // D3DFMT_G32R32F
				return DXGI_FORMAT_R32G32_FLOAT;

			case 116: // D3DFMT_A32B32G32R32F
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
		}

		return DXGI_FORMAT_UNKNOWN;

#undef ISBITMASK
	}

	static DXGI_FORMAT makeSRGB(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		case DXGI_FORMAT_BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM_SRGB;

		case DXGI_FORMAT_BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM_SRGB;

		case DXGI_FORMAT_BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM_SRGB;

		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

		case DXGI_FORMAT_B8G8R8X8_UNORM:
			return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

		case DXGI_FORMAT_BC7_UNORM:
			return DXGI_FORMAT_BC7_UNORM_SRGB;

		default:
			return format;
		}
	}

	void getSurfaceInfo(size_t width, size_t height, DXGI_FORMAT fmt, size_t& outNumBytes, size_t& outRowBytes, size_t& outNumRows)
	{
		bool bc = false;
		bool packed = false;
		bool planar = false;
		size_t bpe = 0;
		switch (fmt)
		{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			bc = true;
			bpe = 8;
			break;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			bc = true;
			bpe = 16;
			break;

		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_YUY2:
			packed = true;
			bpe = 4;
			break;

		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
			packed = true;
			bpe = 8;
			break;

		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_420_OPAQUE:
			planar = true;
			bpe = 2;
			break;

		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
			planar = true;
			bpe = 4;
			break;
		}

		size_t rowBytes;
		if (bc)
		{
			size_t numBlocksWide = 0;
			if (width > 0)
			{
				numBlocksWide = std::max<size_t>(1, (width + 3) / 4);
			}
			size_t numBlocksHigh = 0;
			if (height > 0)
			{
				numBlocksHigh = std::max<size_t>(1, (height + 3) / 4);
			}
			rowBytes = numBlocksWide * bpe;
			outNumRows = numBlocksHigh;
			outNumBytes = rowBytes * numBlocksHigh;
		}
		else if (packed)
		{
			rowBytes = ((width + 1) >> 1) * bpe;
			outNumRows = height;
			outNumBytes = rowBytes * height;
		}
		else if (fmt == DXGI_FORMAT_NV11)
		{
			rowBytes = ((width + 3) >> 2) * 4;
			outNumRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
			outNumBytes = rowBytes * outNumRows;
		}
		else if (planar)
		{
			rowBytes = ((width + 1) >> 1) * bpe;
			outNumRows = height + ((height + 1) >> 1);
			outNumBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
		}
		else
		{
			size_t bpp = bitsPerPixel(fmt);
			rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
			outNumRows = height;
			outNumBytes = rowBytes * height;
		}

		outRowBytes = rowBytes;
	}

	TextureInfo getDDSTextureInfoFromFile(ScopedFile& textureFile)
	{
		TextureInfo textureInfo;
		size_t fileSize = textureFile.size();
		if (fileSize < (sizeof(DDS_HEADER)))
		{
			throw EndOfFileException();
		}

		DDS_HEADER ddsHeader;
		textureFile.read(ddsHeader);

		if (ddsHeader.ddsMagicNumber != DDS_MAGIC || ddsHeader.size != (sizeof(DDS_HEADER) - 4u) ||
			ddsHeader.ddspf.size != sizeof(DDS_PIXELFORMAT))
		{
			throw FormatNotSupportedException();
		}

		uint32_t dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		uint32_t arraySize = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

		if ((ddsHeader.ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == ddsHeader.ddspf.fourCC))
		{
			// Must be long enough for both headers
			if (fileSize < (sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)))
			{
				throw EndOfFileException();
			}

			DDS_HEADER_DXT10 d3d10ext;
			textureFile.read(d3d10ext);

			arraySize = d3d10ext.arraySize;
			if (arraySize == 0u)
			{
				throw FormatNotSupportedException();
			}

			format = d3d10ext.dxgiFormat;

			switch (format)
			{
			case DXGI_FORMAT_AI44:
			case DXGI_FORMAT_IA44:
			case DXGI_FORMAT_P8:
			case DXGI_FORMAT_A8P8:
				throw FormatNotSupportedException();

			default:
				if (bitsPerPixel(format) == 0u)
				{
					throw InvalidHeaderInfoException();
				}
			}

			dimension = d3d10ext.dimension;

			switch (dimension)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				// D3DX writes 1D textures with a fixed Height of 1
				if ((ddsHeader.flags & DDS_HEIGHT) && ddsHeader.height != 1u)
				{
					throw InvalidHeaderInfoException();
				}
				ddsHeader.height = 1;
				ddsHeader.depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				if (d3d10ext.miscFlag & 0x4L) //D3D11_RESOURCE_MISC_TEXTURECUBE = 0x4L
				{
					arraySize *= 6;
				}
				ddsHeader.depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				if (!(ddsHeader.flags & DDS_HEADER_FLAGS_VOLUME))
				{
					throw InvalidHeaderInfoException();
				}

				if (arraySize > 1)
				{
					throw FormatNotSupportedException();
				}
				break;

			default:
				throw FormatNotSupportedException();
			}
		}
		else
		{
			format = getDXGIFormat(ddsHeader.ddspf);

			if (format == DXGI_FORMAT_UNKNOWN)
			{
				throw FormatNotSupportedException();
			}

			if (ddsHeader.flags & DDS_HEADER_FLAGS_VOLUME)
			{
				dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
			else
			{
				if (ddsHeader.caps2 & DDS_CUBEMAP)
				{
					// We require all six faces to be defined
					if ((ddsHeader.caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
					{
						throw InvalidHeaderInfoException();
					}

					arraySize = 6;
				}

				ddsHeader.depth = 1;
				dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

				// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
			}

			assert(bitsPerPixel(format) != 0);
		}

		textureInfo.format = format;
		textureInfo.dimension = static_cast<D3D12_RESOURCE_DIMENSION>(dimension);
		textureInfo.width = ddsHeader.width;
		textureInfo.height = ddsHeader.height;
		textureInfo.depthOrArraySize = arraySize;
		textureInfo.mipLevels = ddsHeader.mipMapCount == 0u ? 1u : ddsHeader.mipMapCount;
		return textureInfo;
	}

	void getDDSTextureInfoFromFile(D3D12_RESOURCE_DESC& textureDesc, ScopedFile& textureFile, D3D12_SHADER_RESOURCE_VIEW_DESC& textureView, bool forceSRGB)
	{
		size_t fileSize = textureFile.size();
		if (fileSize < (sizeof(DDS_HEADER)))
		{
			throw EndOfFileException();
		}

		DDS_HEADER ddsHeader;
		textureFile.read(ddsHeader);

		if (ddsHeader.ddsMagicNumber != DDS_MAGIC || ddsHeader.size != sizeof(DDS_HEADER) ||
			ddsHeader.ddspf.size != sizeof(DDS_PIXELFORMAT))
		{
			throw FormatNotSupportedException();
		}

		uint32_t dimension;
		uint32_t arraySize = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		bool isCubeMap = false;
		if (ddsHeader.mipMapCount == 0u) ddsHeader.mipMapCount = 1u;

		if ((ddsHeader.ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == ddsHeader.ddspf.fourCC))
		{
			// Must be long enough for both headers
			if (fileSize < (sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)))
			{
				throw EndOfFileException();
			}

			DDS_HEADER_DXT10 d3d10ext;
			textureFile.read(d3d10ext);

			arraySize = d3d10ext.arraySize;
			if (arraySize == 0u)
			{
				throw FormatNotSupportedException();
			}

			format = d3d10ext.dxgiFormat;

			switch (format)
			{
			case DXGI_FORMAT_AI44:
			case DXGI_FORMAT_IA44:
			case DXGI_FORMAT_P8:
			case DXGI_FORMAT_A8P8:
				throw FormatNotSupportedException();

			default:
				if (bitsPerPixel(format) == 0u)
				{
					throw InvalidHeaderInfoException();
				}
			}

			dimension = d3d10ext.dimension;

			switch (dimension)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				// D3DX writes 1D textures with a fixed Height of 1
				if ((ddsHeader.flags & DDS_HEIGHT) && ddsHeader.height != 1u)
				{
					throw InvalidHeaderInfoException();
				}
				ddsHeader.height = 1;
				ddsHeader.depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				if (d3d10ext.miscFlag & 0x4L) //D3D11_RESOURCE_MISC_TEXTURECUBE = 0x4L
				{
					arraySize *= 6;
					isCubeMap = true;
				}
				ddsHeader.depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				if (!(ddsHeader.flags & DDS_HEADER_FLAGS_VOLUME))
				{
					throw InvalidHeaderInfoException();
				}

				if (arraySize > 1)
				{
					throw FormatNotSupportedException();
				}
				break;

			default:
				throw FormatNotSupportedException();
			}
		}
		else
		{
			format = getDXGIFormat(ddsHeader.ddspf);

			if (format == DXGI_FORMAT_UNKNOWN)
			{
				throw FormatNotSupportedException();
			}

			if (ddsHeader.flags & DDS_HEADER_FLAGS_VOLUME)
			{
				dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
			else
			{
				if (ddsHeader.caps2 & DDS_CUBEMAP)
				{
					// We require all six faces to be defined
					if ((ddsHeader.caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
					{
						throw InvalidHeaderInfoException();
					}

					arraySize = 6;
					isCubeMap = true;
				}

				ddsHeader.depth = 1;
				dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

				// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
			}

			assert(bitsPerPixel(format) != 0);
		}

		if (forceSRGB) { format = makeSRGB(format); }

		textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = 1u;
		textureDesc.SampleDesc.Quality = 0u;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		textureView.Format = format;
		textureView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// Bound sizes (for security purposes we don't trust DDS file metadata larger than the D3D 11.x hardware requirements)
		if (ddsHeader.mipMapCount > D3D12_REQ_MIP_LEVELS)
		{
			throw SecurityException();
		}

		switch (dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
				(ddsHeader.width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
			{
				throw SecurityException();
			}
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			textureDesc.Width = static_cast<UINT64>(ddsHeader.width);
			textureDesc.Height = 1u;
			textureDesc.DepthOrArraySize = static_cast<UINT16>(arraySize);
			textureDesc.MipLevels = static_cast<UINT16>(ddsHeader.mipMapCount);

			if (arraySize > 1u)
			{
				textureView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				textureView.Texture1DArray.MostDetailedMip = 0u;
				textureView.Texture1DArray.MipLevels = textureDesc.MipLevels;
				textureView.Texture1DArray.FirstArraySlice = 0u;
				textureView.Texture1DArray.ArraySize = arraySize;
				textureView.Texture1DArray.ResourceMinLODClamp = 0.f;
			}
			else
			{
				textureView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
				textureView.Texture1D.MostDetailedMip = 0u;
				textureView.Texture1D.MipLevels = textureDesc.MipLevels;
				textureView.Texture1D.ResourceMinLODClamp = 0.f;
			}

			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			textureDesc.Width = static_cast<UINT64>(ddsHeader.width);
			textureDesc.Height = static_cast<UINT>(ddsHeader.height);
			textureDesc.DepthOrArraySize = static_cast<UINT16>(arraySize);
			textureDesc.MipLevels = static_cast<UINT16>(ddsHeader.mipMapCount);

			if (isCubeMap)
			{
				// This is the right bound because we set arraySize to (NumCubes*6) above
				if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
					(ddsHeader.width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
					(ddsHeader.height > D3D12_REQ_TEXTURECUBE_DIMENSION))
				{
					throw SecurityException();
				}

				if (arraySize > 6)
				{
					textureView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					textureView.TextureCubeArray.MostDetailedMip = 0u;
					textureView.TextureCubeArray.MipLevels = ddsHeader.mipMapCount;
					textureView.TextureCubeArray.First2DArrayFace = 0u;
					// Earlier we set arraySize to (NumCubes * 6)
					textureView.TextureCubeArray.NumCubes = static_cast<UINT>(arraySize / 6u);
					textureView.TextureCubeArray.ResourceMinLODClamp = 0.f;
				}
				else
				{
					textureView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					textureView.TextureCube.MostDetailedMip = 0u;
					textureView.TextureCube.MipLevels = ddsHeader.mipMapCount;
					textureView.TextureCube.ResourceMinLODClamp = 0.f;
				}
			}
			else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
				(ddsHeader.width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
				(ddsHeader.height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
			{
				throw SecurityException();
			}
			else if (arraySize > 1)
			{
				textureView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				textureView.Texture2DArray.MostDetailedMip = 0u;
				textureView.Texture2DArray.MipLevels = ddsHeader.mipMapCount;
				textureView.Texture2DArray.FirstArraySlice = 0u;
				textureView.Texture2DArray.ArraySize = arraySize;
				textureView.Texture2DArray.PlaneSlice = 0u;
				textureView.Texture2DArray.ResourceMinLODClamp = 0.f;
			}
			else
			{
				textureView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				textureView.Texture2D.MostDetailedMip = 0u;
				textureView.Texture2D.MipLevels = ddsHeader.mipMapCount;
				textureView.Texture2D.PlaneSlice = 0u;
				textureView.Texture2D.ResourceMinLODClamp = 0.f;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			if ((arraySize > 1) ||
				(ddsHeader.width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(ddsHeader.height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(ddsHeader.depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
			{
				throw SecurityException();
			}

			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			textureDesc.Width = static_cast<UINT64>(ddsHeader.width);
			textureDesc.Height = static_cast<UINT>(ddsHeader.height);
			textureDesc.DepthOrArraySize = static_cast<UINT16>(ddsHeader.depth);
			textureDesc.MipLevels = static_cast<UINT16>(ddsHeader.mipMapCount);

			textureView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			textureView.Texture3D.MipLevels = ddsHeader.mipMapCount;
			textureView.Texture3D.ResourceMinLODClamp = 0.f;
			break;

		default:
			throw FormatNotSupportedException();
		}
	}

	void LoadDDSTextureFromFile(ID3D12Device* const Device, D3D12_RESOURCE_DESC& TextureDesc, const size_t FileByteSize, D3D12_SHADER_RESOURCE_VIEW_DESC& TextureSRVDesc, HANDLE const TextureFile,
		ID3D12Resource* const UploadTexture, ID3D12Resource* const DefaultTexture, ID3D12GraphicsCommandList* const CopyCommandList, D3D12_CPU_DESCRIPTOR_HANDLE const TextureDecriptorHandle, bool isCubeMap)
	{
		size_t arraySize;
		size_t depth;
		if (TextureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		{
			arraySize = 1;
			depth = TextureDesc.DepthOrArraySize;
		}
		else
		{
			arraySize = TextureDesc.DepthOrArraySize;
			depth = 1;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC textureShaderResourceView;
		textureShaderResourceView.Format = TextureDesc.Format;
		textureShaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (TextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		{
			if (arraySize > 1u)
			{
				textureShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				textureShaderResourceView.Texture1DArray.MostDetailedMip = 0u;
				textureShaderResourceView.Texture1DArray.MipLevels = TextureDesc.MipLevels;
				textureShaderResourceView.Texture1DArray.FirstArraySlice = 0u;
				textureShaderResourceView.Texture1DArray.ArraySize = static_cast<UINT>(arraySize);
				textureShaderResourceView.Texture1DArray.ResourceMinLODClamp = 0.f;
			}
			else
			{
				textureShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
				textureShaderResourceView.Texture1D.MostDetailedMip = 0u;
				textureShaderResourceView.Texture1D.MipLevels = TextureDesc.MipLevels;
				textureShaderResourceView.Texture1D.ResourceMinLODClamp = 0.f;
			}

		}
		break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		{
			if (isCubeMap)
			{
				if (arraySize > 6)
				{
					textureShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					textureShaderResourceView.TextureCubeArray.MostDetailedMip = 0u;
					textureShaderResourceView.TextureCubeArray.MipLevels = TextureDesc.MipLevels;
					textureShaderResourceView.TextureCubeArray.First2DArrayFace = 0u;
					// Earlier we set arraySize to (NumCubes * 6)
					textureShaderResourceView.TextureCubeArray.NumCubes = static_cast<UINT>(arraySize / 6);
					textureShaderResourceView.TextureCubeArray.ResourceMinLODClamp = 0.f;
				}
				else
				{
					textureShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					textureShaderResourceView.TextureCube.MostDetailedMip = 0u;
					textureShaderResourceView.TextureCube.MipLevels = TextureDesc.MipLevels;
					textureShaderResourceView.TextureCube.ResourceMinLODClamp = 0.f;
				}
			}
			else if (arraySize > 1)
			{
				textureShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				textureShaderResourceView.Texture2DArray.MostDetailedMip = 0u;
				textureShaderResourceView.Texture2DArray.MipLevels = TextureDesc.MipLevels;
				textureShaderResourceView.Texture2DArray.FirstArraySlice = 0u;
				textureShaderResourceView.Texture2DArray.ArraySize = static_cast<UINT>(arraySize);
				textureShaderResourceView.Texture2DArray.PlaneSlice = 0u;
				textureShaderResourceView.Texture2DArray.ResourceMinLODClamp = 0.f;
			}
			else
			{
				textureShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				textureShaderResourceView.Texture2D.MostDetailedMip = 0u;
				textureShaderResourceView.Texture2D.MipLevels = TextureDesc.MipLevels;
				textureShaderResourceView.Texture2D.PlaneSlice = 0u;
				textureShaderResourceView.Texture2D.ResourceMinLODClamp = 0.f;
			}
		}
		break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		{
			textureShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			textureShaderResourceView.Texture3D.MostDetailedMip = 0u;
			textureShaderResourceView.Texture3D.MipLevels = TextureDesc.MipLevels;
			textureShaderResourceView.Texture3D.ResourceMinLODClamp = 0.f;

		}
		break;
		}


		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevel;
		HRESULT hr = Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevel, sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));
		if (FAILED(hr)) throw HresultException(hr);
		size_t maxsize;

		switch (featureLevel.MaxSupportedFeatureLevel)
		{
		case D3D_FEATURE_LEVEL_9_1:
		case D3D_FEATURE_LEVEL_9_2:
			if (TextureSRVDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY || TextureSRVDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBE)
			{
				maxsize = 512 /*D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION*/;
			}
			else
			{
				maxsize = (TextureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
					? 256 /*D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION*/
					: 2048 /*D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
			}
			break;

		case D3D_FEATURE_LEVEL_9_3:
			maxsize = (TextureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
				? 256 /*D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION*/
				: 4096 /*D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
			break;

		default: // D3D_FEATURE_LEVEL_10_0 & D3D_FEATURE_LEVEL_10_1
			maxsize = (TextureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
				? 2048 /*D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION*/
				: 8192 /*D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
			break;
		}


		std::unique_ptr<D3D12_SUBRESOURCE_DATA[]> initData(new D3D12_SUBRESOURCE_DATA[TextureDesc.MipLevels * arraySize]);
		std::unique_ptr<uint8_t[]> ByteData(new uint8_t[FileByteSize]);
		DWORD BytesRead;
		ReadFile(TextureFile, ByteData.get(), static_cast<DWORD>(FileByteSize), &BytesRead, nullptr);
		if (BytesRead != FileByteSize) throw IOException();

		size_t NumBytes = 0u;
		size_t RowBytes = 0u;
		size_t NumRows = 0u;
		uint8_t* sourceBits = ByteData.get();
		const uint8_t* sourceBitsEnd = sourceBits + FileByteSize;


		size_t index = 0u;
		for (size_t j = 0u; j < arraySize; j++)
		{
			size_t w = TextureDesc.Width;
			size_t h = TextureDesc.Height;
			size_t d = depth;
			for (size_t i = 0; i < TextureDesc.MipLevels; i++)
			{
				getSurfaceInfo(w, h, TextureDesc.Format, NumBytes, RowBytes, NumRows);

				if ((TextureDesc.MipLevels <= 1) || (w <= maxsize && h <= maxsize && d <= maxsize))
				{
					initData[index].pData = sourceBits;
					initData[index].RowPitch = static_cast<UINT>(RowBytes);
					initData[index].SlicePitch = static_cast<UINT>(NumBytes);
					++index;
				}
				else throw FormatNotSupportedException();

				if (sourceBits + (NumBytes*d) > sourceBitsEnd)
				{
					throw EndOfFileException();
				}

				sourceBits += NumBytes * d;

				w = (w >> 1) > 1 ? w >> 1 : 1;
				h = (h >> 1) > 1 ? h >> 1 : 1;
				d = (d >> 1) > 1 ? d >> 1 : 1;
			}
		}
		if (index <= 0) throw IndexOutOfRangeException();

		const UINT subresourceCount = static_cast<UINT>(TextureDesc.MipLevels * arraySize);
		void* MapSubResource;
		D3D12_RANGE Range = { 0, 0 };
		for (UINT i = 0; i < subresourceCount; i++)
		{
			hr = UploadTexture->Map(i, &Range, &MapSubResource);
			if (FAILED(hr)) throw HresultException(hr);
			hr = UploadTexture->WriteToSubresource(i, nullptr, initData[i].pData, static_cast<UINT>(initData[i].RowPitch), static_cast<UINT>(initData[i].SlicePitch));
			if (FAILED(hr)) throw HresultException(hr);
			UploadTexture->Unmap(i, nullptr);
			
		}
		ByteData.reset();
		CopyCommandList->CopyResource(DefaultTexture, UploadTexture);
		Device->CreateShaderResourceView(DefaultTexture, &TextureSRVDesc, TextureDecriptorHandle);
	}

	void loadSubresourceFromFile(ID3D12Device* const graphicsDevice, uint64_t textureWidth, uint32_t textureHeight, uint32_t textureDepth, DXGI_FORMAT format, uint16_t arrayIndex, uint16_t mipLevel, uint16_t mipLevels,
		ScopedFile TextureFile, void* const UploadBuffer, ID3D12Resource* const destResource, ID3D12GraphicsCommandList* const CopyCommandList, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
	{
		size_t sourceSlicePitch, sourceRowPitch, numRows;
		uint32_t subresouceWidth = (uint32_t)(textureWidth >> mipLevel);
		if (subresouceWidth == 0u) subresouceWidth = 1u;
		uint32_t subresourceHeight = textureHeight >> mipLevel;
		if (subresourceHeight == 0u) subresourceHeight = 1u;
		uint32_t subresourceDepth = textureDepth >> mipLevel;
		if (subresourceDepth == 0u) subresourceDepth = 1u;
		getSurfaceInfo(subresouceWidth, subresourceHeight, format, sourceSlicePitch, sourceRowPitch, numRows);
		size_t destRowPitch = (sourceRowPitch + (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u);//sourceRowPitch is 4 byte aligned but destRowPitch is D3D12_TEXTURE_DATA_PITCH_ALIGNMENT byte aligned

		if (destRowPitch == sourceRowPitch)
		{
			TextureFile.read(UploadBuffer, static_cast<DWORD>(sourceSlicePitch * subresourceDepth));
		}
		else
		{
			//something is wrong here, textures with less than D3D12_TEXTURE_DATA_PITCH_ALIGNMENT width and/or height loot bad
			throw "textures with miplevel width and/or height less than D3D12_TEXTURE_DATA_PITCH_ALIGNMENT don't currently work";
			std::unique_ptr<unsigned char[]> buffer(new unsigned char[sourceSlicePitch * subresourceDepth]);
			TextureFile.read(buffer.get(), static_cast<DWORD>(sourceSlicePitch * subresourceDepth));
			
			unsigned char* source = buffer.get();
			unsigned char* dest = reinterpret_cast<unsigned char*>(UploadBuffer);
			for (uint32_t i = 0u; i < subresourceDepth; ++i)
			{
				for (size_t j = 0u; j < numRows; ++j)
				{
					memcpy(dest, source, sourceRowPitch);
					source += sourceRowPitch;
					dest += destRowPitch;
				}
			}
		}

		D3D12_TEXTURE_COPY_LOCATION destination;
		destination.pResource = destResource;
		destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destination.SubresourceIndex = arrayIndex * mipLevels + mipLevel;
		//destination.PlacedFootprint.Offset = 0u;
		//destination.PlacedFootprint.Footprint.Depth = subresourceDepth;
		//destination.PlacedFootprint.Footprint.Format = format;
		//destination.PlacedFootprint.Footprint.Height = subresourceHeight;
		//destination.PlacedFootprint.Footprint.Width = subresouceWidth;
		//destination.PlacedFootprint.Footprint.RowPitch = static_cast<uint32_t>(destRowPitch);

		D3D12_TEXTURE_COPY_LOCATION UploadBufferLocation;
		UploadBufferLocation.pResource = uploadResource;
		//UploadBufferLocation.SubresourceIndex = 0u;
		UploadBufferLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		UploadBufferLocation.PlacedFootprint.Offset = uploadResourceOffset;
		UploadBufferLocation.PlacedFootprint.Footprint.Depth = subresourceDepth;
		UploadBufferLocation.PlacedFootprint.Footprint.Format = format;
		UploadBufferLocation.PlacedFootprint.Footprint.Height = subresourceHeight;
		UploadBufferLocation.PlacedFootprint.Footprint.Width = subresouceWidth;
		UploadBufferLocation.PlacedFootprint.Footprint.RowPitch = static_cast<uint32_t>(destRowPitch);

		CopyCommandList->CopyTextureRegion(&destination, 0u, 0u, 0u, &UploadBufferLocation, nullptr);
	}
}