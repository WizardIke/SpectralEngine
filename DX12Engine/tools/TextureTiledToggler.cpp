#include "../File.h"
#include "../DDSFileLoader.h"
#include "../InvalidHeaderInfoException.h"
#include "../FormatNotSupportedException.h"
#include <codecvt>
#include <iostream>
#include <string>
#include <memory>

std::wstring stringToWstring(const std::string& t_str)
{
	//setup converter
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.from_bytes(t_str);
}

constexpr uint32_t DDS_RESOURCE_MISC_TEXTURECUBE = 0x4;
constexpr uint32_t DDS_MAGIC = 0x20534444;
constexpr uint32_t DDS_RGB = 0x00000040;
constexpr uint32_t DDS_LUMINANCE = 0x00020000;
constexpr uint32_t DDS_ALPHA = 0x00000002;
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
constexpr uint32_t DDS_CUBEMAP_ALLFACES = (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |
	DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY | DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ);

DDSFileLoader::DdsHeaderDx12 textureInfoToDdsHeaderDx12(const DDSFileLoader::TextureInfo& textureInfo)
{
	DDSFileLoader::DdsHeaderDx12 header;
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
	header.size = (sizeof(DDSFileLoader::DDS_HEADER) - 4u);
	header.width = textureInfo.width;
	header.miscFlag = textureInfo.isCubeMap ? DDS_RESOURCE_MISC_TEXTURECUBE : 0u;
	header.ddspf.size = sizeof(DDSFileLoader::DDS_PIXELFORMAT);
	header.ddspf.RBitMask = 0x00000000;
	header.ddspf.GBitMask = 0x00000000;
	header.ddspf.BBitMask = 0x00000000;
	header.ddspf.ABitMask = 0x00000000;
	header.ddspf.flags = DDS_FOURCC;
	header.ddspf.fourCC = MAKEFOURCC('D', 'X', '1', '0');
	header.ddspf.RGBBitCount = 0;
	constexpr uint32_t DDSCAPS_COMPLEX = 0x8;
	constexpr uint32_t DDSCAPS_MIPMAP = 0x400000;
	constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;
	uint32_t caps = DDSCAPS_TEXTURE;
	if (textureInfo.mipLevels > 1) caps |= DDSCAPS_MIPMAP;
	if (textureInfo.mipLevels > 1 || textureInfo.arraySize > 1) caps |= DDSCAPS_COMPLEX;
	header.caps = caps;
	uint32_t caps2 = 0u;
	if(textureInfo.dimension > D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D) caps2 |= DDSCAPS2_VOLUME;
	if(textureInfo.isCubeMap) caps2 |= DDS_CUBEMAP_ALLFACES;
	header.caps2 = caps2;
	uint32_t flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	if (textureInfo.mipLevels > 1) flags |= DDSD_MIPMAPCOUNT;
	if (textureInfo.dimension > D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D) flags |= DDSD_DEPTH;
	header.flags = flags;
	header.miscFlags2 = DDS_ALPHA_MODE_UNKNOWN;
	header.pitchOrLinearSize = 0u;
	return header;
}

void untile(const DDSFileLoader::TextureInfo& rextureInfo, unsigned char* sourcedata, unsigned char* destData)
{
	uint32_t tileWidth, tileHeight, tileWidthBytes;
	DDSFileLoader::tileWidthAndHeightAndTileWidthInBytes(rextureInfo.format, tileWidth, tileHeight, tileWidthBytes);

	for (size_t currentMipLevel = 0u; currentMipLevel != rextureInfo.mipLevels; ++currentMipLevel)
	{
		uint32_t subresouceWidth = (rextureInfo.width >> currentMipLevel);
		if (subresouceWidth == 0u) subresouceWidth = 1u;
		uint32_t subresourceHeight = rextureInfo.height >> currentMipLevel;
		if (subresourceHeight == 0u) subresourceHeight = 1u;
		uint32_t subresourceDepth = rextureInfo.depth >> currentMipLevel;
		if (subresourceDepth == 0u) subresourceDepth = 1u;

		size_t slicePitch, rowPitch, numRows;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, rextureInfo.format, slicePitch, rowPitch, numRows);

		const size_t numColumns = rowPitch / tileWidthBytes;
		const size_t partialColumnBytes = rowPitch % tileWidthBytes;
		const size_t subresourceSize = slicePitch * subresourceDepth;
		
		uint8_t* currentUploadBuffer = destData;
		const uint8_t* currentDepthSourceStart = sourcedata;
		for (size_t currentDepth = 0u; currentDepth != subresourceDepth; ++currentDepth)
		{
			size_t rowsDown = 0u;
			size_t remainingRows = numRows;
			while (remainingRows != 0u)
			{
				size_t tileHeightOnDisk = tileHeight;
				
				if (remainingRows < tileHeight) tileHeightOnDisk = remainingRows;
				const size_t tileSizeOnDisk = tileHeightOnDisk * tileWidthBytes;

				for (size_t row = 0u; row != tileHeightOnDisk; ++row)
				{
					for (size_t column = 0u; column != numColumns; ++column)
					{
						//copy row of current column
						const uint8_t* currentSourceTileRowStart = currentDepthSourceStart + rowsDown * rowPitch + tileSizeOnDisk * column + tileWidthBytes * row;
						const uint8_t* const currentSourceTileRowEnd = currentSourceTileRowStart + tileWidthBytes;
						for (; currentSourceTileRowStart != currentSourceTileRowEnd;)
						{
							*reinterpret_cast<uint32_t*>(currentUploadBuffer) = *reinterpret_cast<const uint32_t*>(currentSourceTileRowStart);
							currentUploadBuffer += sizeof(uint32_t);
							currentSourceTileRowStart += sizeof(uint32_t);
						}
					}
					//copy part of row of cuurent column, length partialColumnBytes
					const uint8_t* currentSourceTileRowStart = currentDepthSourceStart + rowsDown * rowPitch + tileSizeOnDisk * numColumns + partialColumnBytes * +row;
					const uint8_t* const currentSourceTileRowEnd = currentSourceTileRowStart + partialColumnBytes;
					for (; currentSourceTileRowStart != currentSourceTileRowEnd;)
					{
						*reinterpret_cast<uint32_t*>(currentUploadBuffer) = *reinterpret_cast<const uint32_t*>(currentSourceTileRowStart);
						currentUploadBuffer += sizeof(uint32_t);
						currentSourceTileRowStart += sizeof(uint32_t);
					}
				}

				remainingRows -= tileHeightOnDisk;
				rowsDown += tileHeightOnDisk;
			}
			currentDepthSourceStart += slicePitch;
		}

		destData += subresourceSize;
		sourcedata += subresourceSize;
	}
}

void tile(const DDSFileLoader::TextureInfo& rextureInfo, unsigned char* sourcedata, unsigned char* destData)
{
	uint32_t tileWidth, tileHeight, tileWidthBytes;
	DDSFileLoader::tileWidthAndHeightAndTileWidthInBytes(rextureInfo.format, tileWidth, tileHeight, tileWidthBytes);

	for (size_t currentMipLevel = 0u; currentMipLevel != rextureInfo.mipLevels; ++currentMipLevel)
	{
		uint32_t subresouceWidth = (rextureInfo.width >> currentMipLevel);
		if (subresouceWidth == 0u) subresouceWidth = 1u;
		uint32_t subresourceHeight = rextureInfo.height >> currentMipLevel;
		if (subresourceHeight == 0u) subresourceHeight = 1u;
		uint32_t subresourceDepth = rextureInfo.depth >> currentMipLevel;
		if (subresourceDepth == 0u) subresourceDepth = 1u;

		size_t slicePitch, rowPitch, numRows;
		DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, rextureInfo.format, slicePitch, rowPitch, numRows);

		const size_t numColumns = rowPitch / tileWidthBytes;
		const size_t partialColumnBytes = rowPitch % tileWidthBytes;
		const size_t subresourceSize = slicePitch * subresourceDepth;

		const uint8_t* currentDestBuffer = sourcedata;
		uint8_t* currentDepthSourceStart = destData;
		for (size_t currentDepth = 0u; currentDepth != subresourceDepth; ++currentDepth)
		{
			size_t rowsDown = 0u;
			size_t remainingRows = numRows;
			while (remainingRows != 0u)
			{
				size_t tileHeightOnDisk = tileHeight;
				if (remainingRows < tileHeight) tileHeightOnDisk = remainingRows;
				const size_t tileSizeOnDisk = tileHeightOnDisk * tileWidthBytes;

				for (size_t row = 0u; row != tileHeightOnDisk; ++row)
				{
					for (size_t column = 0u; column != numColumns; ++column)
					{
						//copy row of current column
						uint8_t* currentSourceTileRowStart = currentDepthSourceStart + rowsDown * rowPitch + tileSizeOnDisk * column + tileWidthBytes * row;
						uint8_t* const currentSourceTileRowEnd = currentSourceTileRowStart + tileWidthBytes;
						for (; currentSourceTileRowStart != currentSourceTileRowEnd;)
						{
							*reinterpret_cast<uint32_t*>(currentSourceTileRowStart) = *reinterpret_cast<const uint32_t*>(currentDestBuffer);
							currentDestBuffer += sizeof(uint32_t);
							currentSourceTileRowStart += sizeof(uint32_t);
						}
					}
					//copy part of row of cuurent column, length partialColumnBytes
					uint8_t* currentSourceTileRowStart = currentDepthSourceStart + rowsDown * rowPitch + tileSizeOnDisk * numColumns + partialColumnBytes * + row;
					uint8_t* const currentSourceTileRowEnd = currentSourceTileRowStart + partialColumnBytes;
					for (; currentSourceTileRowStart != currentSourceTileRowEnd;)
					{
						*reinterpret_cast<uint32_t*>(currentSourceTileRowStart) = *reinterpret_cast<const uint32_t*>(currentDestBuffer);
						currentDestBuffer += sizeof(uint32_t);
						currentSourceTileRowStart += sizeof(uint32_t);
					}
				}

				remainingRows -= tileHeightOnDisk;
				rowsDown += tileHeightOnDisk;
			}
		}

		destData += subresourceSize;
		sourcedata += subresourceSize;
	}
}

int main(int argc, char** argv)
{
	if(argc < 2) 
	{
		std::cout << "Needs a file";
		return 1;
	}
	std::wstring fileName = stringToWstring(std::string(argv[1]));
	File file(fileName.c_str(), File::accessRight::genericRead, File::shareMode::readMode, File::creationMode::openExisting);
	DDSFileLoader::TextureInfo textureInfo;
	try 
	{
		textureInfo = DDSFileLoader::getDDSTextureInfoFromFile(file);
	}
	catch (InvalidHeaderInfoException)
	{
		std::cout << "InvalidHeaderInfo Exception\n";
	}
	catch (FormatNotSupportedException)
	{
		std::cout << "FormatNotSupported Exception\n";
	}
	catch (...)
	{
		std::cout << "Exception\n";
	}

	std::cout << "Format: " << textureInfo.format << "\n";
	std::cout << "Diminsion: " << textureInfo.dimension << "\n";
	std::cout << "AraySize: " << textureInfo.arraySize << "\n";
	std::cout << "width: " << textureInfo.width << "\n";
	std::cout << "height: " << textureInfo.height << "\n";
	std::cout << "depth: " << textureInfo.depth << "\n";
	std::cout << "mipLevels: " << textureInfo.mipLevels << "\n";
	std::cout << "isCubeMap: " << textureInfo.isCubeMap << "\n";
	
	size_t dataLength = file.size() - file.getPosition();
	std::unique_ptr<unsigned char[]> data(new unsigned char[dataLength]);
	file.read(data.get(), (uint32_t)dataLength);
	
	const wchar_t* outFileName;
	std::unique_ptr<unsigned char[]> convertedData(new unsigned char[dataLength]);
	if(fileName.size() != 0u && fileName[fileName.size() - 1u] == L's')
	{
		std::cout << "Tiling file\n";
		outFileName = L"converted_file.tile";
		tile(textureInfo, data.get(), convertedData.get());
	}
	else
	{
		std::cout << "Untiling file\n";
		outFileName = L"converted_file.dds";
		untile(textureInfo, data.get(), convertedData.get());
	}
	
	auto header = textureInfoToDdsHeaderDx12(textureInfo);
	std::cout << "Format: " << textureInfo.format << "\n";
	std::cout << "Diminsion: " << header.dimension << "\n";
	std::cout << "AraySize: " << header.arraySize << "\n";
	std::cout << "width: " << header.width << "\n";
	std::cout << "height: " << header.height << "\n";
	std::cout << "depth: " << header.depth << "\n";
	std::cout << "mipLevels: " << header.mipMapCount << "\n";
	std::cout << "isCubeMap: " << ((header.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) != 0u) << "\n";

	File outFile(outFileName, File::accessRight::genericWrite, File::shareMode::writeMode,
		File::creationMode::createNew);
	outFile.write(header);
	outFile.write(convertedData.get(), (uint32_t)dataLength);
	
	std::cout << "Finished";
}

