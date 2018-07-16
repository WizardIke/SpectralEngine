#include "AmbientMusic.h"
#include <SoundDecoder.h>
#include "ThreadResources.h"
#include "GlobalResources.h"
#include <random>

AmbientMusic::AmbientMusic(ThreadResources& executor, GlobalResources& sharedResources, const wchar_t* const * files, size_t fileCount) :
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
}(), 0u, 1.0f, (IXAudio2VoiceCallback*)this),
	bytesRemaining(0u),
	previousTrack(std::numeric_limits<unsigned long long>::max()),
	files(files),
	fileCount(fileCount)
{
	findNextMusic(executor, sharedResources, [](AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		music.update(threadResources, globalResources);
	});
}

AmbientMusic::~AmbientMusic()
{
	file.close();
}

void AmbientMusic::OnBufferEnd(void* pBufferContext)
{
	GlobalResources& sharedResources = *reinterpret_cast<GlobalResources*>(pBufferContext);
	PostMessage(sharedResources.window.native_handle(), 0x8000, (WPARAM)this, reinterpret_cast<LPARAM>(static_cast<void(*)(void* requester, ThreadResources& executor, GlobalResources& sharedResources)>(
		[](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
	{
		executor.taskShedular.backgroundQueue().push({ requester, [](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
		{
			auto& music = *reinterpret_cast<AmbientMusic*>(requester);
			music.update(executor, sharedResources);
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

void AmbientMusic::findNextMusic(ThreadResources& threadResources, GlobalResources& globalResources, void(*callbackFunc)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources))
{
	callback = callbackFunc;

	std::uniform_int_distribution<unsigned long long> distribution(0, fileCount - 1u);
	size_t track = distribution(threadResources.randomNumberGenerator);

	if (track == previousTrack)
	{
		++track;
		if (track == fileCount)
		{
			track = 0u;
		}
	}
	previousTrack = track;

	auto filename = files[track];
	file = globalResources.asynchronousFileManager.openFileForReading<GlobalResources>(globalResources.taskShedular.ioCompletionQueue(), filename);
	filePosition = 4 * 1024u;
	globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, filename, 0, 4 * 1024u, file,
		this, [](void* requester, void* executor, void* sharedResources, const uint8_t* data, File file)
	{
		const SoundDecoder::UncompressedFileInfo& headerInfo = *reinterpret_cast<const SoundDecoder::UncompressedFileInfo*>(data);
		auto& music = *reinterpret_cast<AmbientMusic*>(requester);
		music.bytesRemaining = headerInfo.dataSize;
		music.callback(music, *reinterpret_cast<ThreadResources*>(executor), *reinterpret_cast<GlobalResources*>(sharedResources));
	});
}

void AmbientMusic::update(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int oldBufferLoadingAndIsFirstBuffer = bufferLoadingAndIsFirstBuffer.load(std::memory_order::memory_order_acquire);
	unsigned int newBufferLoadingAndIsFirstBuffer;
	do
	{
		if (oldBufferLoadingAndIsFirstBuffer & 2u) //buffer is still loading, resubmit old buffer.
		{
			XAUDIO2_BUFFER buffer;
			buffer.AudioBytes = rawSoundDataBufferSize / 2u;
			buffer.Flags = 0u;
			buffer.LoopBegin = 0u;
			buffer.LoopCount = 0u;
			buffer.LoopLength = 0u;
			buffer.pContext = &globalResources;
			buffer.PlayBegin = 0u;
			buffer.PlayLength = 0u;
			if (oldBufferLoadingAndIsFirstBuffer & 1u)
			{
				buffer.pAudioData = rawSoundData;
			}
			else
			{
				buffer.pAudioData = rawSoundData + rawSoundDataBufferSize / 2u;
			}
			HRESULT hr = musicPlayer->SubmitSourceBuffer(&buffer);
			if (FAILED(hr)) throw HresultException(hr);
			return;
		}
		newBufferLoadingAndIsFirstBuffer = oldBufferLoadingAndIsFirstBuffer ^ 3u;
	} while (!bufferLoadingAndIsFirstBuffer.compare_exchange_weak(oldBufferLoadingAndIsFirstBuffer, newBufferLoadingAndIsFirstBuffer, std::memory_order::memory_order_acquire));

	if (newBufferLoadingAndIsFirstBuffer & 1u)
	{
		currentBuffer = rawSoundData + rawSoundDataBufferSize / 2u;
	}
	else
	{
		currentBuffer = rawSoundData;
	}

	bytesNeeded = rawSoundDataBufferSize / 2u;
	loadSoundData(*this, threadResources, globalResources);
}

void AmbientMusic::onSoundDataLoadingFinished(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	XAUDIO2_BUFFER buffer;
	buffer.AudioBytes = rawSoundDataBufferSize / 2u;
	buffer.Flags = 0u;
	buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
	buffer.LoopCount = 0u;
	buffer.LoopLength = 0u;
	buffer.pContext = &globalResources;
	buffer.PlayBegin = 0u;
	buffer.PlayLength = 0u;
	buffer.pAudioData = music.currentBuffer;
	HRESULT hr = music.musicPlayer->SubmitSourceBuffer(&buffer);
	if (FAILED(hr)) throw HresultException(hr);

	if (music.currentBuffer == music.rawSoundData)
	{
		music.bufferLoadingAndIsFirstBuffer.store(0u, std::memory_order::memory_order_release);
	}
	else
	{
		music.bufferLoadingAndIsFirstBuffer.store(1u, std::memory_order::memory_order_release);
	}
}

void AmbientMusic::loadSoundData(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
	assert(bytesToCopy != 0u);
	globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, music.files[music.previousTrack], music.filePosition, music.filePosition + bytesToCopy, music.file, &music,
		[](void* requester, void* executor, void* sharedResources, const uint8_t* data, File file)
	{
		auto& music = *reinterpret_cast<AmbientMusic*>(requester);
		size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
		memcpy(music.currentBuffer, data, bytesToCopy);
		music.bytesNeeded -= bytesToCopy;
		music.bytesRemaining -= bytesToCopy;
		music.filePosition += bytesToCopy;

		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);

		if (music.bytesRemaining == 0u)
		{
			music.file.close();
			void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
			if (music.bytesNeeded == 0u)
			{
				callback = onSoundDataLoadingFinished;
			}
			else
			{
				callback = loadSoundData;
			}
			music.findNextMusic(threadResources, globalResources, callback);
		}
		else if (music.bytesNeeded == 0u)
		{
			onSoundDataLoadingFinished(music, threadResources, globalResources);
		}
		else
		{
			loadSoundData(music, threadResources, globalResources);
		}
	});
}

void AmbientMusic::start()
{
	musicPlayer->Start(0u, 0u);
}