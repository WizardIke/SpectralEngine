#include "AmbientMusic.h"
#include <experimental/filesystem>
#include <SharedResources.h>
#include <SoundDecoder.h>
#include <BaseExecutor.h>

AmbientMusic::AmbientMusic(BaseExecutor* const executor) :
	musicPlayer(executor->sharedResources->soundEngine.audioDevice, []()
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
	rawSoundData(new uint8_t[rawSoundDataBufferSize]),
	bytesRemaining(0u),
	previousTrack(std::numeric_limits<unsigned long long>::max())
{
	update(executor);
	musicPlayer->Start(0u, 0u);
}

void AmbientMusic::OnBufferEnd(void* pBufferContext)
{
	SharedResources* sharedResources = reinterpret_cast<SharedResources* const>(pBufferContext);
	PostMessage(sharedResources->window.native_handle(), 0x8000, (WPARAM)this, reinterpret_cast<LPARAM>(static_cast<void(*)(void* requester, BaseExecutor* executor)>([](void* requester, BaseExecutor* executor)
	{
		executor->updateJobQueue().push(Job(requester, [](void* requester, BaseExecutor* executor)
		{
			const auto music = reinterpret_cast<AmbientMusic*>(requester);
			music->update(executor);
		}));
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

void AmbientMusic::update(BaseExecutor* const executor)
{
	if (!bytesRemaining)
	{
		std::experimental::filesystem::directory_iterator start{ L"../DemoGame/Music/Peaceful" };
		unsigned long long numTracks = std::distance(std::experimental::filesystem::directory_iterator{ L"../DemoGame/Music/Peaceful" }, std::experimental::filesystem::directory_iterator{}); //start iterator is invalid after function

		std::uniform_int_distribution<unsigned long long> distribution(0, numTracks - 1u);
		unsigned long long track = distribution(executor->randomNumberGenerator);

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

	XAUDIO2_BUFFER buffer;
	if (bytesRemaining < rawSoundDataBufferSize / 2u)
	{
		buffer.AudioBytes = static_cast<uint32_t>(bytesRemaining);
		fclose(currentFile);
		bytesRemaining = 0u;
	}
	else
	{
		buffer.AudioBytes = rawSoundDataBufferSize / 2u;
		bytesRemaining -= rawSoundDataBufferSize / 2u;
	}
	buffer.Flags = 0u;
	buffer.LoopBegin = 0u;
	buffer.LoopCount = 0u;
	buffer.LoopLength = 0u;
	if (firstBuffer)
	{
		SoundDecoder::loadWaveFileChunk(currentFile, rawSoundData.get(), buffer.AudioBytes);
		buffer.pAudioData = rawSoundData.get();
		firstBuffer = false;
	}
	else
	{
		SoundDecoder::loadWaveFileChunk(currentFile, rawSoundData.get() + rawSoundDataBufferSize / 2u, buffer.AudioBytes);
		buffer.pAudioData = rawSoundData.get() + rawSoundDataBufferSize / 2u;
		firstBuffer = true;
	}
	buffer.pContext = executor->sharedResources;
	buffer.PlayBegin = 0u;
	buffer.PlayLength = 0u;

	HRESULT hr = musicPlayer->SubmitSourceBuffer(&buffer);
	if (FAILED(hr)) throw HresultException(hr);
}