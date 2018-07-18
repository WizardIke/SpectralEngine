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
	fileCount(fileCount),
	globalResources(sharedResources)
{
}

AmbientMusic::~AmbientMusic()
{
	file.close();
}

void AmbientMusic::OnBufferEnd(void* pBufferContext)
{
	BufferDescriptor* bufferDescriptor = reinterpret_cast<BufferDescriptor*>(pBufferContext);
	if (bufferDescriptor == &bufferDescriptors[3])
	{
		PostMessage(globalResources.window.native_handle(), 0x8000, (WPARAM)bufferDescriptor, reinterpret_cast<LPARAM>(static_cast<void(*)(void* requester, ThreadResources& executor, GlobalResources& sharedResources)>(
			[](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
		{
			executor.taskShedular.backgroundQueue().push({ requester, [](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
			{
				BufferDescriptor* bufferDescriptor = reinterpret_cast<BufferDescriptor*>(requester);
				sharedResources.asynchronousFileManager.discard(bufferDescriptor->filename.load(std::memory_order::memory_order_relaxed), bufferDescriptor->filePositionStart, bufferDescriptor->filePositionEnd);
				bufferDescriptor->filename.store(nullptr, std::memory_order::memory_order_release);
			} });
		})));
	}
	else
	{
		PostMessage(globalResources.window.native_handle(), 0x8000, (WPARAM)this, reinterpret_cast<LPARAM>(static_cast<void(*)(void* requester, ThreadResources& executor, GlobalResources& sharedResources)>(
			[](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
		{
			executor.taskShedular.backgroundQueue().push({ requester, [](void* requester, ThreadResources& executor, GlobalResources& sharedResources)
			{
				auto& music = *reinterpret_cast<AmbientMusic*>(requester);
				music.update(executor, sharedResources);
			} });
		})));
	}
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
			buffer.pAudioData = bufferDescriptors[oldBufferLoadingAndIsFirstBuffer & 1u].data;
			HRESULT hr = musicPlayer->SubmitSourceBuffer(&buffer);
			if (FAILED(hr)) throw HresultException(hr);
			return;
		}
		newBufferLoadingAndIsFirstBuffer = oldBufferLoadingAndIsFirstBuffer ^ 3u;
	} while (!bufferLoadingAndIsFirstBuffer.compare_exchange_weak(oldBufferLoadingAndIsFirstBuffer, newBufferLoadingAndIsFirstBuffer, std::memory_order::memory_order_acquire));

	currentBuffer = &bufferDescriptors[!(newBufferLoadingAndIsFirstBuffer & 1u)];

	bytesNeeded = rawSoundDataBufferSize / 2u;
	loadSoundData(*this, threadResources, globalResources);
}

void AmbientMusic::onSoundDataLoadingFinished(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	submitBuffer(music.musicPlayer, music.currentBuffer, music.currentBuffer->data, music.currentBuffer->filePositionEnd - music.currentBuffer->filePositionStart);

	constexpr bool powerOfTwo = (sizeof(BufferDescriptor) & (sizeof(BufferDescriptor) - 1)) == 0;
#if powerOfTwo
	music.bufferLoadingAndIsFirstBuffer.store(music.currentBuffer - &music.bufferDescriptors[0], std::memory_order::memory_order_release);
#else
	if (music.currentBuffer == &music.bufferDescriptors[0])
	{
		music.bufferLoadingAndIsFirstBuffer.store(0u, std::memory_order::memory_order_release);
	}
	else
	{
		music.bufferLoadingAndIsFirstBuffer.store(1u, std::memory_order::memory_order_release);
	}
#endif
}

void AmbientMusic::loadSoundData(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
	assert(bytesToCopy != 0u);
	globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, music.files[music.previousTrack], music.filePosition, music.filePosition + bytesToCopy, music.file, &music,
		[](void* requester, void* executor, void* sharedResources, const uint8_t* data, File file)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
		auto& music = *reinterpret_cast<AmbientMusic*>(requester);

		size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
		if (music.currentBuffer->filename.load(std::memory_order::memory_order_relaxed) != nullptr) { globalResources.asynchronousFileManager.discard(music.currentBuffer->filename.load(std::memory_order::memory_order_relaxed),
			music.currentBuffer->filePositionStart, music.currentBuffer->filePositionEnd); }
		music.currentBuffer->data = data;
		music.currentBuffer->filename.store(music.files[music.previousTrack], std::memory_order::memory_order_relaxed);
		music.currentBuffer->filePositionStart = music.filePosition;
		music.currentBuffer->filePositionEnd = music.filePosition + bytesToCopy;
		
		music.bytesNeeded -= bytesToCopy;
		music.bytesRemaining -= bytesToCopy;
		music.filePosition += bytesToCopy;

		if (music.bytesRemaining != 0u)
		{
			onSoundDataLoadingFinished(music, threadResources, globalResources);
		}
		else
		{
			music.file.close();
			void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
			if (music.bytesNeeded == 0u && music.bufferDescriptors[3].filename.load(std::memory_order::memory_order_acquire) != nullptr)
			{
				callback = onSoundDataLoadingFinished;
			}
			else
			{
				music.bufferDescriptors[3].filename.store(music.currentBuffer->filename.load(std::memory_order::memory_order_relaxed), std::memory_order::memory_order_relaxed);
				music.bufferDescriptors[3].filePositionStart = music.currentBuffer->filePositionStart;
				music.bufferDescriptors[3].filePositionEnd = music.currentBuffer->filePositionEnd;
				submitBuffer(music.musicPlayer, &music.bufferDescriptors[3], data, bytesToCopy);
				callback = loadSoundDataPart2;
			}
			music.findNextMusic(threadResources, globalResources, callback);
		}
	});
}

void AmbientMusic::loadSoundDataPart2(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
	assert(bytesToCopy != 0u);
	globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, music.files[music.previousTrack], music.filePosition, music.filePosition + bytesToCopy, music.file, &music,
		[](void* requester, void* executor, void* sharedResources, const uint8_t* data, File file)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
		auto& music = *reinterpret_cast<AmbientMusic*>(requester);

		size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
		music.currentBuffer->data = data;
		music.currentBuffer->filename = music.files[music.previousTrack];
		music.currentBuffer->filePositionStart = music.filePosition;
		music.currentBuffer->filePositionEnd = music.filePosition + bytesToCopy;

		music.bytesRemaining -= bytesToCopy;
		music.filePosition += bytesToCopy;

		if (music.bytesRemaining == 0u)
		{
			music.file.close();
			music.findNextMusic(threadResources, globalResources, onSoundDataLoadingFinished);
		}
		else
		{
			onSoundDataLoadingFinished(music, threadResources, globalResources);
		}
	});
}

void AmbientMusic::start(ThreadResources& executor, GlobalResources& sharedResources)
{
	findNextMusic(executor, sharedResources, [](AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		music.update(threadResources, globalResources);
		music.musicPlayer->Start(0u, 0u);
	});
}

void AmbientMusic::submitBuffer(IXAudio2SourceVoice* musicPlayer, void* context, const uint8_t* data, std::size_t length)
{
	XAUDIO2_BUFFER buffer;
	buffer.AudioBytes = (UINT32)length;
	buffer.Flags = 0u;
	buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
	buffer.LoopCount = 0u;
	buffer.LoopLength = 0u;
	buffer.pContext = context;
	buffer.PlayBegin = 0u;
	buffer.PlayLength = 0u;
	buffer.pAudioData = data;
	HRESULT hr = musicPlayer->SubmitSourceBuffer(&buffer);
	if (FAILED(hr)) throw HresultException(hr);
}