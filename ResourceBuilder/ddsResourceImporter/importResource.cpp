#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <array>

namespace
{
	constexpr std::array<uint8_t, 4u> makeFourCC(uint8_t ch0, uint8_t ch1, uint8_t ch2, uint8_t ch3)
	{
		return { ch0, ch1, ch2, ch3 };
	}

	enum DXGI_FORMAT
	{
		DXGI_FORMAT_UNKNOWN = 0,
		DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
		DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
		DXGI_FORMAT_R32G32B32A32_UINT = 3,
		DXGI_FORMAT_R32G32B32A32_SINT = 4,
		DXGI_FORMAT_R32G32B32_TYPELESS = 5,
		DXGI_FORMAT_R32G32B32_FLOAT = 6,
		DXGI_FORMAT_R32G32B32_UINT = 7,
		DXGI_FORMAT_R32G32B32_SINT = 8,
		DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
		DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
		DXGI_FORMAT_R16G16B16A16_UNORM = 11,
		DXGI_FORMAT_R16G16B16A16_UINT = 12,
		DXGI_FORMAT_R16G16B16A16_SNORM = 13,
		DXGI_FORMAT_R16G16B16A16_SINT = 14,
		DXGI_FORMAT_R32G32_TYPELESS = 15,
		DXGI_FORMAT_R32G32_FLOAT = 16,
		DXGI_FORMAT_R32G32_UINT = 17,
		DXGI_FORMAT_R32G32_SINT = 18,
		DXGI_FORMAT_R32G8X24_TYPELESS = 19,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
		DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
		DXGI_FORMAT_R10G10B10A2_UNORM = 24,
		DXGI_FORMAT_R10G10B10A2_UINT = 25,
		DXGI_FORMAT_R11G11B10_FLOAT = 26,
		DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
		DXGI_FORMAT_R8G8B8A8_UNORM = 28,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
		DXGI_FORMAT_R8G8B8A8_UINT = 30,
		DXGI_FORMAT_R8G8B8A8_SNORM = 31,
		DXGI_FORMAT_R8G8B8A8_SINT = 32,
		DXGI_FORMAT_R16G16_TYPELESS = 33,
		DXGI_FORMAT_R16G16_FLOAT = 34,
		DXGI_FORMAT_R16G16_UNORM = 35,
		DXGI_FORMAT_R16G16_UINT = 36,
		DXGI_FORMAT_R16G16_SNORM = 37,
		DXGI_FORMAT_R16G16_SINT = 38,
		DXGI_FORMAT_R32_TYPELESS = 39,
		DXGI_FORMAT_D32_FLOAT = 40,
		DXGI_FORMAT_R32_FLOAT = 41,
		DXGI_FORMAT_R32_UINT = 42,
		DXGI_FORMAT_R32_SINT = 43,
		DXGI_FORMAT_R24G8_TYPELESS = 44,
		DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
		DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
		DXGI_FORMAT_R8G8_TYPELESS = 48,
		DXGI_FORMAT_R8G8_UNORM = 49,
		DXGI_FORMAT_R8G8_UINT = 50,
		DXGI_FORMAT_R8G8_SNORM = 51,
		DXGI_FORMAT_R8G8_SINT = 52,
		DXGI_FORMAT_R16_TYPELESS = 53,
		DXGI_FORMAT_R16_FLOAT = 54,
		DXGI_FORMAT_D16_UNORM = 55,
		DXGI_FORMAT_R16_UNORM = 56,
		DXGI_FORMAT_R16_UINT = 57,
		DXGI_FORMAT_R16_SNORM = 58,
		DXGI_FORMAT_R16_SINT = 59,
		DXGI_FORMAT_R8_TYPELESS = 60,
		DXGI_FORMAT_R8_UNORM = 61,
		DXGI_FORMAT_R8_UINT = 62,
		DXGI_FORMAT_R8_SNORM = 63,
		DXGI_FORMAT_R8_SINT = 64,
		DXGI_FORMAT_A8_UNORM = 65,
		DXGI_FORMAT_R1_UNORM = 66,
		DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
		DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
		DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
		DXGI_FORMAT_BC1_TYPELESS = 70,
		DXGI_FORMAT_BC1_UNORM = 71,
		DXGI_FORMAT_BC1_UNORM_SRGB = 72,
		DXGI_FORMAT_BC2_TYPELESS = 73,
		DXGI_FORMAT_BC2_UNORM = 74,
		DXGI_FORMAT_BC2_UNORM_SRGB = 75,
		DXGI_FORMAT_BC3_TYPELESS = 76,
		DXGI_FORMAT_BC3_UNORM = 77,
		DXGI_FORMAT_BC3_UNORM_SRGB = 78,
		DXGI_FORMAT_BC4_TYPELESS = 79,
		DXGI_FORMAT_BC4_UNORM = 80,
		DXGI_FORMAT_BC4_SNORM = 81,
		DXGI_FORMAT_BC5_TYPELESS = 82,
		DXGI_FORMAT_BC5_UNORM = 83,
		DXGI_FORMAT_BC5_SNORM = 84,
		DXGI_FORMAT_B5G6R5_UNORM = 85,
		DXGI_FORMAT_B5G5R5A1_UNORM = 86,
		DXGI_FORMAT_B8G8R8A8_UNORM = 87,
		DXGI_FORMAT_B8G8R8X8_UNORM = 88,
		DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
		DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
		DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
		DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
		DXGI_FORMAT_BC6H_TYPELESS = 94,
		DXGI_FORMAT_BC6H_UF16 = 95,
		DXGI_FORMAT_BC6H_SF16 = 96,
		DXGI_FORMAT_BC7_TYPELESS = 97,
		DXGI_FORMAT_BC7_UNORM = 98,
		DXGI_FORMAT_BC7_UNORM_SRGB = 99,
		DXGI_FORMAT_AYUV = 100,
		DXGI_FORMAT_Y410 = 101,
		DXGI_FORMAT_Y416 = 102,
		DXGI_FORMAT_NV12 = 103,
		DXGI_FORMAT_P010 = 104,
		DXGI_FORMAT_P016 = 105,
		DXGI_FORMAT_420_OPAQUE = 106,
		DXGI_FORMAT_YUY2 = 107,
		DXGI_FORMAT_Y210 = 108,
		DXGI_FORMAT_Y216 = 109,
		DXGI_FORMAT_NV11 = 110,
		DXGI_FORMAT_AI44 = 111,
		DXGI_FORMAT_IA44 = 112,
		DXGI_FORMAT_P8 = 113,
		DXGI_FORMAT_A8P8 = 114,
		DXGI_FORMAT_B4G4R4A4_UNORM = 115,

		DXGI_FORMAT_P208 = 130,
		DXGI_FORMAT_V208 = 131,
		DXGI_FORMAT_V408 = 132,


		DXGI_FORMAT_FORCE_UINT = 0xffffffff
	};

	enum D3D12_RESOURCE_DIMENSION
	{
		D3D12_RESOURCE_DIMENSION_UNKNOWN = 0,
		D3D12_RESOURCE_DIMENSION_BUFFER = 1,
		D3D12_RESOURCE_DIMENSION_TEXTURE1D = 2,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D = 3,
		D3D12_RESOURCE_DIMENSION_TEXTURE3D = 4
	};

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
		std::array<uint8_t, 4u>    fourCC;
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

	constexpr uint32_t DDS_RESOURCE_MISC_TEXTURECUBE = 0x4;
	constexpr uint32_t DDS_MAGIC = 0x20534444;
	constexpr uint32_t DDS_RGB = 0x00000040;
	constexpr uint32_t DDS_LUMINANCE = 0x00020000;
	constexpr uint32_t DDS_ALPHA = 0x00000002;
	constexpr uint32_t DDS_ALPHAPIXELS = 0x00000001;
	constexpr uint32_t DDS_FOURCC = 0x00000004;
	constexpr uint32_t DDS_ALPHA_MODE_UNKNOWN = 0x0;

	constexpr uint32_t DDSD_CAPS = 0x1;
	constexpr uint32_t DDSD_HEIGHT = 0x2;
	constexpr uint32_t DDSD_WIDTH = 0x4;
	constexpr uint32_t DDSD_PITCH = 0x8;
	constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
	constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;
	constexpr uint32_t DDSD_LINEARSIZE = 0x80000;
	constexpr uint32_t DDSD_DEPTH = 0x800000;

	constexpr uint32_t DDSCAPS2_CUBEMAP = 0x200;
	constexpr uint32_t DDSCAPS2_VOLUME = 0x200000;
	constexpr uint32_t DDS_CUBEMAP_POSITIVEX = 0x00000600; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
	constexpr uint32_t DDS_CUBEMAP_NEGATIVEX = 0x00000a00; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
	constexpr uint32_t DDS_CUBEMAP_POSITIVEY = 0x00001200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
	constexpr uint32_t DDS_CUBEMAP_NEGATIVEY = 0x00002200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
	constexpr uint32_t DDS_CUBEMAP_POSITIVEZ = 0x00004200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
	constexpr uint32_t DDS_CUBEMAP_NEGATIVEZ = 0x00008200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ
	constexpr uint32_t DDS_CUBEMAP_ALLFACES = (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX | DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY | DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ);


	DDS_PIXELFORMAT dxgiFormatToDdsPixelFormat(DXGI_FORMAT format)
	{
		DDS_PIXELFORMAT ddspf;
		ddspf.size = sizeof(DDS_PIXELFORMAT);
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			ddspf.RBitMask = 0x000000ff;
			ddspf.GBitMask = 0x0000ff00;
			ddspf.BBitMask = 0x00ff0000;
			ddspf.ABitMask = 0xff000000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 32;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			ddspf.RBitMask = 0x00ff0000;
			ddspf.GBitMask = 0x0000ff00;
			ddspf.BBitMask = 0x000000ff;
			ddspf.ABitMask = 0xff000000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 32;
			break;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			ddspf.RBitMask = 0x00ff0000;
			ddspf.GBitMask = 0x0000ff00;
			ddspf.BBitMask = 0x000000ff;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 32;
			break;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			// Note that many common DDS reader/writers (including D3DX) swap the
			// the RED/BLUE masks for 10:10:10:2 formats. We assumme
			// below that the 'backwards' header mask is being used since it is most
			// likely written by D3DX. The more robust solution is to use the 'DX10'
			// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

			// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
			ddspf.RBitMask = 0x3ff00000;
			ddspf.GBitMask = 0x000ffc00;
			ddspf.BBitMask = 0x000003ff;
			ddspf.ABitMask = 0xc0000000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 32;
			break;
		case DXGI_FORMAT_R16G16_UNORM:
			ddspf.RBitMask = 0x0000ffff;
			ddspf.GBitMask = 0xffff0000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 32;
			break;
		case DXGI_FORMAT_B5G5R5A1_UNORM:
			ddspf.RBitMask = 0x7c00;
			ddspf.GBitMask = 0x03e0;
			ddspf.BBitMask = 0x001f;
			ddspf.ABitMask = 0x8000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 16;
			break;
		case DXGI_FORMAT_B5G6R5_UNORM:
			ddspf.RBitMask = 0xf800;
			ddspf.GBitMask = 0x07e0;
			ddspf.BBitMask = 0x001f;
			ddspf.ABitMask = 0x0000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 16;
			break;
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			ddspf.RBitMask = 0x0f00;
			ddspf.GBitMask = 0x00f0;
			ddspf.BBitMask = 0x000f;
			ddspf.ABitMask = 0xf000;
			ddspf.flags = DDS_RGB;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 16;
			break;
		case DXGI_FORMAT_R8_UNORM:
			ddspf.RBitMask = 0x000000ff;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_LUMINANCE;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 8;
			break;
		case DXGI_FORMAT_R16_UNORM:
			ddspf.RBitMask = 0x0000ffff;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_LUMINANCE;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 16;
			break;
		case DXGI_FORMAT_R8G8_UNORM:
			ddspf.RBitMask = 0x000000ff;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x0000ff00;
			ddspf.flags = DDS_LUMINANCE;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 16;
			break;
		case DXGI_FORMAT_A8_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x000000ff;
			ddspf.flags = DDS_ALPHA;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 8;
			break;
		case DXGI_FORMAT_BC1_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('D', 'X', 'T', '1');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_BC2_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('D', 'X', 'T', '3');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_BC3_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('D', 'X', 'T', '5');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_BC4_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('B', 'C', '4', 'U');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_BC4_SNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('B', 'C', '4', 'S');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_BC5_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('B', 'C', '5', 'U');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_BC5_SNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('B', 'C', '5', 'S');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('R', 'G', 'B', 'G');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('G', 'R', 'G', 'B');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_YUY2:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = makeFourCC('Y', 'U', 'Y', '2');
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 36, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R16G16B16A16_SNORM:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 110, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R16_FLOAT:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 111, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R16G16_FLOAT:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 112, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 113, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R32_FLOAT:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 114, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 115, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = DDS_FOURCC;
			ddspf.fourCC = { 116, 0, 0, 0 };
			ddspf.RGBBitCount = 0;
			break;
		default:
			ddspf.RBitMask = 0x00000000;
			ddspf.GBitMask = 0x00000000;
			ddspf.BBitMask = 0x00000000;
			ddspf.ABitMask = 0x00000000;
			ddspf.flags = 0u;
			ddspf.fourCC = { 0u, 0u, 0u, 0u };
			ddspf.RGBBitCount = 0u;
			break;
		}
		return ddspf;
	}

	size_t bitsPerPixel(DXGI_FORMAT fmt)
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
		constexpr auto ISBITMASK = [](const DDS_PIXELFORMAT& ddpf, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
		{
			return ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a;
		};
		if (ddpf.flags & DDS_RGB)
		{
			// Note that sRGB formats are written using the "DX10" extended header

			switch (ddpf.RGBBitCount)
			{
			case 32:
				if (ISBITMASK(ddpf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
				{
					return DXGI_FORMAT_R8G8B8A8_UNORM;
				}

				if (ISBITMASK(ddpf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
				{
					return DXGI_FORMAT_B8G8R8A8_UNORM;
				}

				if (ISBITMASK(ddpf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
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
				if (ISBITMASK(ddpf, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
				{
					return DXGI_FORMAT_R10G10B10A2_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

				if (ISBITMASK(ddpf, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R16G16_UNORM;
				}

				if (ISBITMASK(ddpf, 0xffffffff, 0x00000000, 0x00000000, 0x00000000))
				{
					// Only 32-bit color channel format in D3D9 was R32F
					return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
				}
				break;

			case 24:
				// No 24bpp DXGI formats aka D3DFMT_R8G8B8
				break;

			case 16:
				if (ISBITMASK(ddpf, 0x7c00, 0x03e0, 0x001f, 0x8000))
				{
					return DXGI_FORMAT_B5G5R5A1_UNORM;
				}
				if (ISBITMASK(ddpf, 0xf800, 0x07e0, 0x001f, 0x0000))
				{
					return DXGI_FORMAT_B5G6R5_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

				if (ISBITMASK(ddpf, 0x0f00, 0x00f0, 0x000f, 0xf000))
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
				if (ISBITMASK(ddpf, 0x000000ff, 0x00000000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
			}

			if (16 == ddpf.RGBBitCount)
			{
				if (ISBITMASK(ddpf, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
				}
				if (ISBITMASK(ddpf, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
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
		else if (ddpf.flags & DDS_ALPHAPIXELS)
		{
			if (8 == ddpf.RGBBitCount)
			{
				return DXGI_FORMAT_A8_UNORM;
			}
		}
		else if (ddpf.flags & DDS_FOURCC)
		{
			if (makeFourCC('D', 'X', 'T', '1') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC1_UNORM;
			}
			if (makeFourCC('D', 'X', 'T', '3') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (makeFourCC('D', 'X', 'T', '5') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			// While pre-mulitplied alpha isn't directly supported by the DXGI formats,
			// they are basically the same as these BC formats so they can be mapped
			if (makeFourCC('D', 'X', 'T', '2') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (makeFourCC('D', 'X', 'T', '4') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			if (makeFourCC('A', 'T', 'I', '1') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (makeFourCC('B', 'C', '4', 'U') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (makeFourCC('B', 'C', '4', 'S') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_SNORM;
			}

			if (makeFourCC('A', 'T', 'I', '2') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (makeFourCC('B', 'C', '5', 'U') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (makeFourCC('B', 'C', '5', 'S') == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_SNORM;
			}

			// BC6H and BC7 are written using the "DX10" extended header

			if (makeFourCC('R', 'G', 'B', 'G') == ddpf.fourCC)
			{
				return DXGI_FORMAT_R8G8_B8G8_UNORM;
			}
			if (makeFourCC('G', 'R', 'G', 'B') == ddpf.fourCC)
			{
				return DXGI_FORMAT_G8R8_G8B8_UNORM;
			}

			if (makeFourCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
			{
				return DXGI_FORMAT_YUY2;
			}

			// Check for D3DFORMAT enums being set here
			switch (ddpf.fourCC[0])
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
	}

	TextureInfo getDDSTextureInfoFromFile(std::istream& textureFile)
	{
		TextureInfo textureInfo;

		DDS_HEADER ddsHeader;
		textureFile.read(reinterpret_cast<char*>(&ddsHeader), sizeof(ddsHeader));
		if (!textureFile)
		{
			throw std::runtime_error("file size too small for header");
		}

		if (ddsHeader.ddsMagicNumber != DDS_MAGIC || ddsHeader.size != (sizeof(DDS_HEADER) - 4u) ||
			ddsHeader.ddspf.size != sizeof(DDS_PIXELFORMAT))
		{
			throw std::runtime_error("invalid magic numbers");
		}

		uint32_t dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		uint32_t arraySize = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		bool isCubeMap = false;

		if ((ddsHeader.ddspf.flags & DDS_FOURCC) && (makeFourCC('D', 'X', '1', '0') == ddsHeader.ddspf.fourCC))
		{
			DDS_HEADER_DXT10 d3d10ext;
			textureFile.read(reinterpret_cast<char*>(&d3d10ext), sizeof(DDS_HEADER_DXT10));
			if (!textureFile)
			{
				throw std::runtime_error("file size too small for expended header");
			}

			arraySize = d3d10ext.arraySize;
			if (arraySize == 0u)
			{
				throw std::runtime_error("array size must be at least one");
			}

			format = d3d10ext.dxgiFormat;

			switch (format)
			{
			case DXGI_FORMAT_AI44:
			case DXGI_FORMAT_IA44:
			case DXGI_FORMAT_P8:
			case DXGI_FORMAT_A8P8:
				throw std::runtime_error("invalid format");

			default:
				if (bitsPerPixel(format) == 0u)
				{
					throw std::runtime_error("invalid format");
				}
			}

			dimension = d3d10ext.dimension;

			switch (dimension)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				// D3DX writes 1D textures with a fixed Height of 1
				if ((ddsHeader.flags & DDSD_HEIGHT) && ddsHeader.height != 1u)
				{
					throw std::runtime_error("1D texture must have a height of one");
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
				if (!(ddsHeader.flags & DDSD_DEPTH))
				{
					throw std::runtime_error("3D texture must have depth flag set");
				}

				if (arraySize > 1)
				{
					throw std::runtime_error("3D texture can't have an array size greater than one");
				}
				break;

			default:
				throw std::runtime_error("unknown texture diminsion");
			}
		}
		else
		{
			format = getDXGIFormat(ddsHeader.ddspf);

			if (format == DXGI_FORMAT_UNKNOWN)
			{
				throw std::runtime_error("unknown format");
			}

			if (ddsHeader.flags & DDSD_DEPTH)
			{
				dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
			else
			{
				if (ddsHeader.caps2 & DDSCAPS2_CUBEMAP)
				{
					// We require all six faces to be defined
					if ((ddsHeader.caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
					{
						throw std::runtime_error("cubemaps must have all faces");
					}
					isCubeMap = true;
					arraySize = 6;
				}

				ddsHeader.depth = 1;
				dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

				// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
			}

			if (bitsPerPixel(format) == 0u)
			{
				throw std::runtime_error("invalid format");
			}
		}

		textureInfo.format = format;
		textureInfo.dimension = static_cast<D3D12_RESOURCE_DIMENSION>(dimension);
		textureInfo.width = ddsHeader.width;
		textureInfo.height = ddsHeader.height;
		textureInfo.arraySize = arraySize;
		textureInfo.depth = ddsHeader.depth;
		textureInfo.mipLevels = ddsHeader.mipMapCount == 0u ? 1u : ddsHeader.mipMapCount;
		textureInfo.isCubeMap = isCubeMap;
		return textureInfo;
	}
}

extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
bool importResource(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
	bool(*importResource)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath))
{
	try
	{
		std::ifstream file(baseInputPath / relativeInputPath, std::ios::binary);
		if (!file)
		{
			std::cout << "failed to open input file\n";
			return false;
		}
		auto textureInfo = getDDSTextureInfoFromFile(file);

		auto currentPos = file.tellg();
		file.seekg(0, file.end);
		auto length = file.tellg();
		file.seekg(currentPos, file.beg);

		auto dataLength = length - currentPos;
		std::unique_ptr<char[]> data(new char[dataLength]);
		file.read(data.get(), dataLength);
		file.close();


		DdsHeaderDx12 header;
		header.arraySize = textureInfo.isCubeMap ? textureInfo.arraySize / 6u : textureInfo.arraySize;
		header.caps3 = 0u;
		header.caps4 = 0u;
		header.ddsMagicNumber = DDS_MAGIC;
		header.depth = textureInfo.depth;
		header.dimension = (uint32_t)textureInfo.dimension;
		header.dxgiFormat = textureInfo.format;
		header.height = textureInfo.height;
		header.mipMapCount = textureInfo.mipLevels;
		std::fill(std::begin(header.reserved1), std::end(header.reserved1), 0u);
		header.reserved2 = 0u;
		header.size = (sizeof(DDS_HEADER) - 4u);
		header.width = textureInfo.width;
		header.miscFlag = textureInfo.isCubeMap ? DDS_RESOURCE_MISC_TEXTURECUBE : 0u;
		header.ddspf.size = sizeof(DDS_PIXELFORMAT);
		header.ddspf.RBitMask = 0x00000000;
		header.ddspf.GBitMask = 0x00000000;
		header.ddspf.BBitMask = 0x00000000;
		header.ddspf.ABitMask = 0x00000000;
		header.ddspf.flags = DDS_FOURCC;
		header.ddspf.fourCC = makeFourCC('D', 'X', '1', '0');
		header.ddspf.RGBBitCount = 0;
		constexpr uint32_t DDSCAPS_COMPLEX = 0x8;
		constexpr uint32_t DDSCAPS_MIPMAP = 0x400000;
		constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;
		uint32_t caps = DDSCAPS_TEXTURE;
		if (textureInfo.mipLevels > 1) caps |= DDSCAPS_MIPMAP;
		if (textureInfo.mipLevels > 1 || textureInfo.arraySize > 1) caps |= DDSCAPS_COMPLEX;
		header.caps = caps;
		uint32_t caps2 = 0u;
		if (textureInfo.dimension > D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D) caps2 |= DDSCAPS2_VOLUME;
		if (textureInfo.isCubeMap) caps2 |= DDS_CUBEMAP_ALLFACES;
		header.caps2 = caps2;
		uint32_t flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		if (textureInfo.mipLevels > 1) flags |= DDSD_MIPMAPCOUNT;
		if (textureInfo.dimension > D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D) flags |= DDSD_DEPTH;
		header.flags = flags;
		header.miscFlags2 = DDS_ALPHA_MODE_UNKNOWN;
		header.pitchOrLinearSize = 0u;

		auto outputPath = baseOutputPath / relativeInputPath;
		outputPath += ".data";
		const auto& outputDirectory = outputPath.parent_path();
		if (!std::filesystem::exists(outputDirectory))
		{
			std::filesystem::create_directories(outputDirectory);
		}
		std::ofstream outFile(outputPath, std::ios::binary);

		outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
		outFile.write(data.get(), (uint32_t)dataLength);
		outFile.close();

		std::cout << "done\n";
	}
	catch (const std::exception& e)
	{
		std::cout << "failed " << e.what() << "\n";
		return false;
	}
	catch (...)
	{
		std::cout << "failed\n";
		return false;
	}
	return true;
}