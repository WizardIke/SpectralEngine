#include <filesystem>
#include <memory>
#include <fstream>
#include <iostream>
#include <cstdint>

namespace
{
	class SoundDecoder
	{
	public:
		class UncompressedFileInfo
		{
		public:
			uint16_t audioFormat;
			uint16_t numChannels;
			uint32_t sampleRate;
			uint32_t bytesPerSecond;
			uint16_t blockAlign;
			uint16_t bitsPerSample;
			uint32_t dataSize;
		};

		static UncompressedFileInfo getWaveFileInfo(std::ifstream& file)
		{
			UncompressedFileInfo waveFileInfo;

			union
			{
				char subChunkId[4];
				char chunkId[4];
				char format[4];
			};

			union
			{
				uint32_t subChunkSize;
				uint32_t chunkSize;
			};

			file.read(chunkId, sizeof(chunkId));

			if ((chunkId[0] != 'R') | (chunkId[1] != 'I') |
				(chunkId[2] != 'F') | (chunkId[3] != 'F'))
			{
				throw std::runtime_error("invalid format");
			}

			file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));

			file.read(format, sizeof(format));

			// Check that the file format is the WAVE format.
			if ((format[0] != 'W') | (format[1] != 'A') |
				(format[2] != 'V') | (format[3] != 'E'))
			{
				throw std::runtime_error("invalid format");
			}

			while (true)
			{
				file.read(subChunkId, sizeof(subChunkId));
				file.read(reinterpret_cast<char*>(&subChunkSize), sizeof(subChunkSize));

				if (subChunkId[0] == 'f' && subChunkId[1] == 'm' && subChunkId[2] == 't' && subChunkId[3] == ' ')
				{
					file.read(reinterpret_cast<char*>(&waveFileInfo.audioFormat), sizeof(waveFileInfo.audioFormat));
					file.read(reinterpret_cast<char*>(&waveFileInfo.numChannels), sizeof(waveFileInfo.numChannels));
					file.read(reinterpret_cast<char*>(&waveFileInfo.sampleRate), sizeof(waveFileInfo.sampleRate));
					file.read(reinterpret_cast<char*>(&waveFileInfo.bytesPerSecond), sizeof(waveFileInfo.bytesPerSecond));
					file.read(reinterpret_cast<char*>(&waveFileInfo.blockAlign), sizeof(waveFileInfo.blockAlign));
					file.read(reinterpret_cast<char*>(&waveFileInfo.bitsPerSample), sizeof(waveFileInfo.bitsPerSample));
					break;
				}
				else
				{
					file.seekg(subChunkSize, file.cur); //this subChunk in unimportant - skip it
				}
			}

			while (true)
			{
				file.read(subChunkId, sizeof(subChunkId));
				file.read(reinterpret_cast<char*>(&subChunkSize), sizeof(subChunkSize));
				if (subChunkId[0] == 'd' && subChunkId[1] == 'a' && subChunkId[2] == 't' && subChunkId[3] == 'a')
				{
					waveFileInfo.dataSize = subChunkSize;
					break;
				}
				else
				{
					file.seekg(subChunkSize, file.cur); //this subChunk in unimportant - skip it
				}
			}

			return waveFileInfo;
		}

		static void loadWaveFileChunk(std::ifstream& file, char* buffer, std::size_t numBytesToRead)
		{
			file.read(buffer, numBytesToRead);
		}
	};

	struct FileInfo
	{
		SoundDecoder::UncompressedFileInfo info;
		unsigned char padding[1024 * 4 - sizeof(SoundDecoder::UncompressedFileInfo)];
	};
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
		auto inputPath = baseInputPath / relativeInputPath;
		auto outputPath = baseOutputPath / relativeInputPath;
		outputPath += ".data";

		if (std::filesystem::exists(outputPath) && std::filesystem::last_write_time(outputPath) > std::filesystem::last_write_time(inputPath))
		{
			return true;
		}

		std::cout << "importing " << inputPath.string() << "\n";
		std::ifstream file{ inputPath, std::ios::binary };
		FileInfo fileInfo = {};
		fileInfo.info = SoundDecoder::getWaveFileInfo(file);
		std::unique_ptr<char[]> data(new char[fileInfo.info.dataSize]);
		SoundDecoder::loadWaveFileChunk(file, data.get(), fileInfo.info.dataSize);
		file.close();

		const auto& outputDirectory = outputPath.parent_path();
		if (!std::filesystem::exists(outputDirectory))
		{
			std::filesystem::create_directories(outputDirectory);
		}
		std::ofstream outFile{ outputPath, std::ios::binary };
		outFile.write(reinterpret_cast<char*>(&fileInfo), sizeof(FileInfo));
		outFile.write(data.get(), fileInfo.info.dataSize);
	}
	catch (...)
	{
		std::cerr << "failed\n";
		return false;
	}

	return true;
}