#include "DDSFileLoader.h"
#include "ScopedFile.h"
#include <string>
#include <codecvt>
#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "Wrong number of args, expects a file name";
		return 1;
	}
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring fileName = converter.from_bytes(argv[1]);
	ScopedFile file(fileName.c_str(), ScopedFile::accessRight::genericRead, ScopedFile::shareMode::readMode, ScopedFile::creationMode::openExisting, nullptr);
	auto textureInfo = DDSFileLoader::getDDSTextureInfoFromFile(file);
	if (textureInfo.dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D && textureInfo.format == DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM)
	{
		size_t firstSubresourceSize = textureInfo.width * textureInfo.height;
		std::unique_ptr<unsigned char[]> data(new unsigned char[]);
		file.read(data.get(), firstSubresourceSize);
		std::wstring outFileName = fileName;
		outFileName.erase(outFileName.size() - 4u, 4u);
		outFileName += L"_converted.dds";
		ScopedFile outFile(outFileName.c_str(), ScopedFile::accessRight::genericWrite, ScopedFile::shareMode::writeMode, ScopedFile::creationMode::createNew, nullptr);
		unsigned int mipCount = 1u;
		auto width = textureInfo.width;
		auto height = textureInfo.height;
		while (width != 1u || height != 1)
		{
			width >>= 1u;
			if (width == 0u) width = 1u;
			height >>= 1u;
			if (height == 0u) height = 1u;
			++mipCount;
		}
		textureInfo.mipLevels = mipCount;
		size_t subresourceSize = firstSubresourceSize;
		DDSFileLoader::writeDDSTextureInfoToFile(outFile, textureInfo);
		width = textureInfo.width;
		height = textureInfo.height;
		//right subresourece
		while (width != 1u || height != 1)
		{
			width >>= 1u;
			if (width == 0u) width = 1u;
			height >>= 1u;
			if (height == 0u) height = 1u;
			subresourceSize = width * height;
			//generate mip level
			//right subresourece
		}
	}
	else
	{
		std::cout << "format not supported";
		return 2;
	}
}