#include "AmbientMusic.h"
#include <SoundDecoder.h>
#include "ThreadResources.h"
#include "GlobalResources.h"
#include <random>

AmbientMusic::AmbientMusic(ThreadResources& executor, GlobalResources& sharedResources, const wchar_t* const * files, std::size_t fileCount) :
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
	IOCompletionPacket updateTask;
	if (bufferDescriptor == &bufferDescriptors[3])
	{
		updateTask.numberOfBytesTransfered = 0u;
		updateTask.overlapped = reinterpret_cast<LPOVERLAPPED>(bufferDescriptor);
		updateTask.completionKey = reinterpret_cast<ULONG_PTR>(static_cast<bool(*)(void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)>(
			[](void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)
		{
			auto& threadResources = *reinterpret_cast<ThreadResources*>(executor);
			auto& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
			BufferDescriptor* bufferDescriptor = reinterpret_cast<BufferDescriptor*>(overlapped);
			globalResources.asynchronousFileManager.discard(bufferDescriptor->filename.load(std::memory_order::memory_order_relaxed), bufferDescriptor->filePositionStart, bufferDescriptor->filePositionEnd);
			bufferDescriptor->filename.store(nullptr, std::memory_order::memory_order_release);
			return true;
		}));
	}
	else
	{
		updateTask.numberOfBytesTransfered = 0u;
		updateTask.overlapped = reinterpret_cast<LPOVERLAPPED>(this);
		updateTask.completionKey = reinterpret_cast<ULONG_PTR>(static_cast<bool(*)(void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)>(
			[](void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)
		{
			auto& music = *reinterpret_cast<AmbientMusic*>(overlapped);
			auto& threadResources = *reinterpret_cast<ThreadResources*>(executor);
			auto& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
			music.update(threadResources, globalResources);
			return true;
		}));
	}
	globalResources.ioCompletionQueue.push(updateTask);
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
	std::size_t track = distribution(threadResources.randomNumberGenerator);

	if (track == previousTrack)
	{
		++track;
		if (track == fileCount)
		{
			track = 0u;
		}
	}
	previousTrack = track;
	filePosition = 4 * 1024u;

	this->filename = files[track];
	this->file = globalResources.asynchronousFileManager.openFileForReading<GlobalResources>(globalResources.ioCompletionQueue, filename);
	this->start = 0u;
	this->end = 4 * 1024u;
	this->fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
	{
		const SoundDecoder::UncompressedFileInfo& headerInfo = *reinterpret_cast<const SoundDecoder::UncompressedFileInfo*>(buffer);
		auto& music = static_cast<AmbientMusic&>(request);
		auto& globalResources = *reinterpret_cast<GlobalResources*>(gr);
		music.bytesRemaining = headerInfo.dataSize;
		globalResources.asynchronousFileManager.discard(music.files[music.previousTrack], 0, 4 * 1024u);
		music.callback(music, *reinterpret_cast<ThreadResources*>(tr), *reinterpret_cast<GlobalResources*>(gr));
	};
}

void AmbientMusic::update(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int oldBufferLoadingAndIsFirstBuffer = bufferLoadingAndIsFirstBuffer.load(std::memory_order::memory_order_acquire);
	unsigned int newBufferLoadingAndIsFirstBuffer;
	do
	{
		if (oldBufferLoadingAndIsFirstBuffer & 2u) //buffer is still loading, resubmit old buffer.
		{
			submitBuffer(musicPlayer, &globalResources, bufferDescriptors[!(oldBufferLoadingAndIsFirstBuffer & 1u)].data, rawSoundDataBufferSize);
			return;
		}
		newBufferLoadingAndIsFirstBuffer = oldBufferLoadingAndIsFirstBuffer ^ 3u;
	} while (!bufferLoadingAndIsFirstBuffer.compare_exchange_weak(oldBufferLoadingAndIsFirstBuffer, newBufferLoadingAndIsFirstBuffer, std::memory_order::memory_order_acquire));

	currentBuffer = &bufferDescriptors[newBufferLoadingAndIsFirstBuffer & 1u];

	bytesNeeded = rawSoundDataBufferSize;
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
	std::size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
	assert(bytesToCopy != 0u);
	music.start = music.filePosition;
	music.end = music.filePosition + bytesToCopy;
	music.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
		auto& music = static_cast<AmbientMusic&>(request);

		std::size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
		if(music.currentBuffer->filename.load(std::memory_order::memory_order_relaxed) != nullptr)
		{
			globalResources.asynchronousFileManager.discard(music.currentBuffer->filename.load(std::memory_order::memory_order_relaxed),
				music.currentBuffer->filePositionStart, music.currentBuffer->filePositionEnd);
		}
		music.currentBuffer->data = buffer;
		music.currentBuffer->filename.store(music.files[music.previousTrack], std::memory_order::memory_order_relaxed);
		music.currentBuffer->filePositionStart = music.filePosition;
		music.currentBuffer->filePositionEnd = music.filePosition + bytesToCopy;

		music.bytesNeeded -= bytesToCopy;
		music.bytesRemaining -= bytesToCopy;
		music.filePosition += bytesToCopy;

		if(music.bytesRemaining != 0u)
		{
			onSoundDataLoadingFinished(music, threadResources, globalResources);
		}
		else
		{
			music.file.close();
			void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
			if(music.bytesNeeded == 0u || music.bufferDescriptors[3].filename.load(std::memory_order::memory_order_acquire) != nullptr)
			{
				callback = onSoundDataLoadingFinished;
			}
			else
			{
				music.bufferDescriptors[3].filename.store(music.currentBuffer->filename.load(std::memory_order::memory_order_relaxed), std::memory_order::memory_order_relaxed);
				music.bufferDescriptors[3].filePositionStart = music.currentBuffer->filePositionStart;
				music.bufferDescriptors[3].filePositionEnd = music.currentBuffer->filePositionEnd;
				submitBuffer(music.musicPlayer, &music.bufferDescriptors[3], buffer, bytesToCopy);
				callback = loadSoundDataPart2;
			}
			music.findNextMusic(threadResources, globalResources, callback);
		}
	};
	globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &music);
}

void AmbientMusic::loadSoundDataPart2(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	std::size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
	assert(bytesToCopy != 0u);
	music.start = music.filePosition;
	music.end = music.filePosition + bytesToCopy;
	music.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
		auto& music = static_cast<AmbientMusic&>(request);

		std::size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
		music.currentBuffer->data = buffer;
		music.currentBuffer->filename = music.files[music.previousTrack];
		music.currentBuffer->filePositionStart = music.filePosition;
		music.currentBuffer->filePositionEnd = music.filePosition + bytesToCopy;

		music.bytesRemaining -= bytesToCopy;
		music.filePosition += bytesToCopy;

		if(music.bytesRemaining == 0u)
		{
			music.file.close();
			music.findNextMusic(threadResources, globalResources, onSoundDataLoadingFinished);
		}
		else
		{
			onSoundDataLoadingFinished(music, threadResources, globalResources);
		}
	};
	globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &music);
}

void AmbientMusic::start(ThreadResources& executor, GlobalResources& sharedResources)
{
	findNextMusic(executor, sharedResources, [](AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		music.startImpl(threadResources, globalResources);
	});
}

void AmbientMusic::startImpl(ThreadResources& threadResources, GlobalResources& globalResources)
{
	//TODO load and submit first buffer then music.update(threadResources, globalResources);
	unsigned int oldBufferLoadingAndIsFirstBuffer = bufferLoadingAndIsFirstBuffer.load(std::memory_order::memory_order_relaxed);
	unsigned int newBufferLoadingAndIsFirstBuffer = oldBufferLoadingAndIsFirstBuffer ^ 3u;
	bufferLoadingAndIsFirstBuffer.store(newBufferLoadingAndIsFirstBuffer, std::memory_order::memory_order_relaxed);
	currentBuffer = &bufferDescriptors[!(newBufferLoadingAndIsFirstBuffer & 1u)];
	bytesNeeded = rawSoundDataBufferSize;

	std::size_t bytesToCopy = std::min(bytesNeeded, bytesRemaining);
	assert(bytesToCopy != 0u);
	this->start = filePosition;
	this->end = filePosition + bytesToCopy;
	this->fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
	{
		auto& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		auto& globalResources = *reinterpret_cast<GlobalResources*>(gr);
		auto& music = static_cast<AmbientMusic&>(request);

		std::size_t bytesToCopy = std::min(music.bytesNeeded, music.bytesRemaining);
		assert(music.currentBuffer->filename.load(std::memory_order::memory_order_relaxed) == nullptr);
		music.currentBuffer->data = buffer;
		music.currentBuffer->filename.store(music.files[music.previousTrack], std::memory_order::memory_order_relaxed);
		music.currentBuffer->filePositionStart = music.filePosition;
		music.currentBuffer->filePositionEnd = music.filePosition + bytesToCopy;

		music.bytesNeeded -= bytesToCopy;
		music.bytesRemaining -= bytesToCopy;
		music.filePosition += bytesToCopy;

		if(music.bytesRemaining != 0u)
		{
			onFirstBufferFinishedLoading(music, threadResources, globalResources);
		}
		else
		{
			music.file.close();
			music.findNextMusic(threadResources, globalResources, onFirstBufferFinishedLoading);
		}
	};
	globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, this);
}

void AmbientMusic::onFirstBufferFinishedLoading(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
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
	music.musicPlayer->Start(0u, 0u);
	music.update(threadResources, globalResources);
}

void AmbientMusic::submitBuffer(IXAudio2SourceVoice* musicPlayer, void* context, const unsigned char* data, std::size_t length)
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