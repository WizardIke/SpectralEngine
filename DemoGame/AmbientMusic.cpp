#include "AmbientMusic.h"
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#include <SoundDecoder.h>
#include "ThreadResources.h"
#include "GlobalResources.h"
#include <random>

AmbientMusic::AmbientMusic(ThreadResources& executor, GlobalResources& sharedResources) :
	musicPlayer(sharedResources.soundEngine.audioDevice, []()
{
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100u;
	waveFormat.wBitsPerSample = 16u;
	waveFormat.nChannels = 2u;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8u) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0u;
	return waveFormat;
}(), 0u, 1.0f, this),
	bytesRemaining(0u),
	previousTrack(std::numeric_limits<unsigned long long>::max())
{
	findNextMusic(executor);
	update(executor, sharedResources);
}

void AmbientMusic::OnBufferEnd(void* pBufferContext)
{
	GlobalResources& sharedResources = *reinterpret_cast<GlobalResources*>(pBufferContext);
	PostMessage(sharedResources.window.native_handle(), 0x8000, (WPARAM)this, reinterpret_cast<LPARAM>(static_cast<void(*)(void* requester, ThreadResources& executor, GlobalResources& sharedResources)>(
		[](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
	{
		executor.taskShedular.update1NextQueue().push({requester, [](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
		{
			const auto music = reinterpret_cast<AmbientMusic*>(requester);
			music->update(executor, sharedResources);
		} });
	})));
}

void AmbientMusic::OnBufferStart(void* pBufferContext)
{
}

void AmbientMusic::OnLoopEnd(void* pBufferContext)
{
}

void AmbientMusic::OnStreamEnd()
{
}

void AmbientMusic::OnVoiceError(void* pBufferContext, HRESULT Error)
{
}

void AmbientMusic::OnVoiceProcessingPassEnd()
{
}

void AmbientMusic::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{
}

void AmbientMusic::findNextMusic(ThreadResources& executor)
{
	filesystem::directory_iterator start{ L"../DemoGame/Music/Peaceful" };
	unsigned long long numTracks = std::distance(filesystem::directory_iterator{ L"../DemoGame/Music/Peaceful" }, filesystem::directory_iterator{}); //start iterator is invalid after function

	std::uniform_int_distribution<unsigned long long> distribution(0, numTracks - 1u);
	unsigned long long track = distribution(executor.randomNumberGenerator);

	if (track == previousTrack)
	{
		++track;
		if (track == numTracks)
		{
			track = 0u;
		}
	}

	for (unsigned long long i = 0u; i < track; ++i, ++start);

	auto filename = start->path().c_str();
	_wfopen_s(&currentFile, filename, L"rb");
	if (!currentFile) throw false;

	SoundDecoder::WaveFileInfo headerInfo = SoundDecoder::getWaveFileInfo(currentFile);

	bytesRemaining = headerInfo.dataSize;
}

void AmbientMusic::update(ThreadResources& executor, GlobalResources& sharedResources)
{
	XAUDIO2_BUFFER buffer;
	buffer.AudioBytes = rawSoundDataBufferSize / 2u;
	buffer.Flags = 0u;
	buffer.LoopBegin = 0u;
	buffer.LoopCount = 0u;
	buffer.LoopLength = 0u;
	buffer.pContext = &sharedResources;
	buffer.PlayBegin = 0u;
	buffer.PlayLength = 0u;

	uint8_t* currentBuffer;
	if (firstBuffer)
	{
		buffer.pAudioData = rawSoundData;
		currentBuffer = rawSoundData;
		firstBuffer = false;
	}
	else
	{
		buffer.pAudioData = rawSoundData + rawSoundDataBufferSize / 2u;
		currentBuffer = rawSoundData + rawSoundDataBufferSize / 2u;
		firstBuffer = true;
	}


	size_t bytesNeeded = rawSoundDataBufferSize / 2u;
	size_t bytesToCopy = std::min(bytesNeeded, bytesRemaining);
	while (true)
	{
		SoundDecoder::loadWaveFileChunk(currentFile, currentBuffer, bytesToCopy);
		bytesNeeded -= bytesToCopy;
		if (bytesNeeded == 0u)
		{
			bytesRemaining -= bytesToCopy;
			if (bytesRemaining == 0u)
			{
				fclose(currentFile);
				findNextMusic(executor);
			}
			break;
		}
		fclose(currentFile);
		findNextMusic(executor);
		bytesToCopy = std::min(bytesNeeded, bytesRemaining);
	}

	HRESULT hr = musicPlayer->SubmitSourceBuffer(&buffer);
	if (FAILED(hr)) throw HresultException(hr);
}

void AmbientMusic::start()
{
	musicPlayer->Start(0u, 0u);
}