#pragma once
#include <stdint.h>
#include <stdio.h>
#include "IOException.h"
#include "FormatNotSupportedException.h"

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

	static UncompressedFileInfo getWaveFileInfo(FILE* const file)
	{
		UncompressedFileInfo waveFileInfo;
		size_t count;

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

		count = fread(chunkId, sizeof(chunkId), 1u, file);
		if (count != 1u) throw IOException();

		if ((chunkId[0] != 'R') | (chunkId[1] != 'I') |
			(chunkId[2] != 'F') | (chunkId[3] != 'F'))
		{
			throw FormatNotSupportedException();
		}

		count = fread(&chunkSize, sizeof(chunkSize), 1u, file);
		if (count != 1u) throw IOException();

		count = fread(format, sizeof(format), 1u, file);
		if (count != 1u) throw IOException();

		// Check that the file format is the WAVE format.
		if ((format[0] != 'W') | (format[1] != 'A') |
			(format[2] != 'V') | (format[3] != 'E'))
		{
			throw FormatNotSupportedException();
		}

		while (true)
		{
			count = fread(subChunkId, sizeof(subChunkId), 1u, file);
			if (count != 1u) throw IOException();

			count = fread(&subChunkSize, sizeof(subChunkSize), 1u, file);
			if (count != 1u) throw IOException();

			if (subChunkId[0] == 'f' && subChunkId[1] == 'm' && subChunkId[2] == 't' && subChunkId[3] == ' ')
			{
				count = fread(&waveFileInfo.audioFormat, sizeof(waveFileInfo.audioFormat), 1u, file);
				if (count != 1u) throw IOException();

				count = fread(&waveFileInfo.numChannels, sizeof(waveFileInfo.numChannels), 1u, file);
				if (count != 1u) throw IOException();

				count = fread(&waveFileInfo.sampleRate, sizeof(waveFileInfo.sampleRate), 1u, file);
				if (count != 1u) throw IOException();

				count = fread(&waveFileInfo.bytesPerSecond, sizeof(waveFileInfo.bytesPerSecond), 1u, file);
				if (count != 1u) throw IOException();

				count = fread(&waveFileInfo.blockAlign, sizeof(waveFileInfo.blockAlign), 1u, file);
				if (count != 1u) throw IOException();

				count = fread(&waveFileInfo.bitsPerSample, sizeof(waveFileInfo.bitsPerSample), 1u, file);
				if (count != 1u) throw IOException();

				break;
			}
			else
			{
				fseek(file, subChunkSize, SEEK_CUR); //this subChunk in unimportant - skip it
			}
		}

		while (true)
		{
			count = fread(subChunkId, sizeof(subChunkId), 1u, file);
			if (count != 1u) throw IOException();

			count = fread(&subChunkSize, sizeof(subChunkSize), 1u, file);
			if (count != 1u) throw IOException();

			if (subChunkId[0] == 'd' && subChunkId[1] == 'a' && subChunkId[2] == 't' && subChunkId[3] == 'a')
			{
				waveFileInfo.dataSize = subChunkSize;
				break;
			}
			else
			{
				fseek(file, subChunkSize, SEEK_CUR);//this subChunk in unimportant - skip it
			}
		}

		return waveFileInfo;
	}

	static void loadWaveFileChunk(FILE* const file, uint8_t* buffer, size_t numBytesToRead)
	{
		size_t count = fread(buffer, numBytesToRead, 1u, file);
		if (count != 1u) throw IOException();
	}
};