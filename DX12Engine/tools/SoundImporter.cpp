#include "../SoundDecoder.h"
#include <memory>
#include <stdio.h>

struct FileInfo
{
	SoundDecoder::UncompressedFileInfo info;
	unsigned char padding[1024 * 4 - sizeof(SoundDecoder::UncompressedFileInfo)];
};

int main(int argc, char** argv)
{
	if(argc < 2) return 1;
	FILE* file = fopen(argv[1], "rb");
	if(file == nullptr) return 2;
	FileInfo fileInfo = {};
	fileInfo.info = SoundDecoder::getWaveFileInfo(file);
	std::unique_ptr<unsigned char[]> data(new unsigned char[fileInfo.info.dataSize]);
	SoundDecoder::loadWaveFileChunk(file, data.get(), fileInfo.info.dataSize);
	fclose(file);
	
	file = fopen("converted_file.sound", "wb");
	if(file == nullptr) return 3;
	size_t count = fwrite(&fileInfo, sizeof(FileInfo), 1u, file);
	if(count != 1u) return 4;
	count = fwrite(data.get(), fileInfo.info.dataSize, 1u, file);
	if(count != 1u) return 5;
	printf("Finished");
}