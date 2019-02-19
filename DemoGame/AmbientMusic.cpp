#include "AmbientMusic.h"
#include <SoundDecoder.h>
#include "ThreadResources.h"
#include "GlobalResources.h"
#include <random>

AmbientMusic::AmbientMusic(GlobalResources& sharedResources, const wchar_t* const * files, std::size_t fileCount) :
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
	globalResourcesRef(sharedResources)
{
	bufferDescriptors[0].music = this;
	bufferDescriptors[1].music = this;

	extraBuffer.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void*, void*)
	{
		auto& bufferDescriptor = static_cast<ExtraBuffer&>(request);
		bufferDescriptor.inUse.store(false, std::memory_order::memory_order_release);
	};
	extraBuffer.music = this;

	infoRequest.music = this;
	infoRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* data)
	{
		const SoundDecoder::UncompressedFileInfo& headerInfo = *reinterpret_cast<const SoundDecoder::UncompressedFileInfo*>(data);
		auto& music = *static_cast<InfoRequest&>(request).music;
		auto& threadResources = *static_cast<ThreadResources*>(tr);
		auto& globalResources = *static_cast<GlobalResources*>(gr);
		music.bytesRemaining = headerInfo.dataSize;
		globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
	};
	infoRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
	{
		auto& music = *static_cast<InfoRequest&>(request).music;
		auto& threadResources = *static_cast<ThreadResources*>(tr);
		auto& globalResources = *static_cast<GlobalResources*>(gr);
		music.callback(music, threadResources, globalResources);
	};
	infoRequest.start = 0u;
	infoRequest.end = 4 * 1024u;
}

AmbientMusic::~AmbientMusic()
{
	file.close();
}

void AmbientMusic::OnBufferEnd(void* pBufferContext)
{
	auto* bufferDescriptor = static_cast<Buffer*>(pBufferContext);
	IOCompletionPacket updateTask;
	if (bufferDescriptor == &extraBuffer)
	{
		updateTask.numberOfBytesTransfered = 0u;
		updateTask.overlapped = static_cast<LPOVERLAPPED>(bufferDescriptor);
		updateTask.completionKey = reinterpret_cast<ULONG_PTR>(static_cast<bool(*)(void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)>(
			[](void* tr, void* sharedResources, DWORD, LPOVERLAPPED overlapped)
		{
			auto& threadResources = *static_cast<ThreadResources*>(tr);
			auto& globalResources = *static_cast<GlobalResources*>(sharedResources);
			auto bufferDescriptor = static_cast<ExtraBuffer*>(overlapped);
			globalResources.asynchronousFileManager.discard(bufferDescriptor, threadResources, globalResources);
			return true;
		}));
	}
	else
	{
		updateTask.numberOfBytesTransfered = 0u;
		updateTask.overlapped = reinterpret_cast<LPOVERLAPPED>(this);
		updateTask.completionKey = reinterpret_cast<ULONG_PTR>(static_cast<bool(*)(void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)>(
			[](void* executor, void* sharedResources, DWORD, LPOVERLAPPED overlapped)
		{
			auto& music = *reinterpret_cast<AmbientMusic*>(overlapped);
			auto& threadResources = *static_cast<ThreadResources*>(executor);
			auto& globalResources = *static_cast<GlobalResources*>(sharedResources);
			music.update(threadResources, globalResources);
			return true;
		}));
	}
	globalResourcesRef.ioCompletionQueue.push(updateTask);
}

void AmbientMusic::OnBufferStart(void*)
{
}

void AmbientMusic::OnLoopEnd(void*)
{
}

void AmbientMusic::OnStreamEnd()
{
}

void AmbientMusic::OnVoiceError(void*, HRESULT)
{
}

void AmbientMusic::OnVoiceProcessingPassEnd()
{
}

void AmbientMusic::OnVoiceProcessingPassStart(UINT32)
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

	auto& request = infoRequest;
	request.filename = files[track];
	request.file = globalResources.asynchronousFileManager.openFileForReading<GlobalResources>(globalResources.ioCompletionQueue, request.filename);
	file = request.file;
	globalResources.asynchronousFileManager.readFile(&request, threadResources, globalResources);
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

	loadSoundData(*this, threadResources, globalResources);
}

void AmbientMusic::onSoundDataLoadingFinished(AmbientMusic& music, ThreadResources&, GlobalResources&)
{
	submitBuffer(music.musicPlayer, music.currentBuffer, music.currentBuffer->data, music.currentBuffer->end - music.currentBuffer->start);

	constexpr bool powerOfTwo = (sizeof(AsynchronousFileManager::ReadRequest) & (sizeof(AsynchronousFileManager::ReadRequest) - 1)) == 0;
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
	auto& currentBuffer = *music.currentBuffer;
	currentBuffer.file = music.file;
	currentBuffer.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* data)
	{
		auto& threadResources = *static_cast<ThreadResources*>(tr);
		auto& globalResources = *static_cast<GlobalResources*>(gr);
		auto& currentBuffer = static_cast<Buffer&>(request);
		auto& music = *currentBuffer.music;
		currentBuffer.data = data;

		std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
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
			if(rawSoundDataBufferSize == bytesToCopy || music.extraBuffer.inUse.load(std::memory_order::memory_order_acquire))
			{
				callback = onSoundDataLoadingFinished;
			}
			else
			{
				music.extraBuffer.inUse.store(true, std::memory_order::memory_order_relaxed);
				music.extraBuffer.filename = currentBuffer.filename;
				music.extraBuffer.start = currentBuffer.start;
				music.extraBuffer.end = currentBuffer.end;
				submitBuffer(music.musicPlayer, &music.extraBuffer, currentBuffer.data, bytesToCopy);
				callback = loadSoundDataPart2;
			}
			music.findNextMusic(threadResources, globalResources, callback);
		}
	};
	if(currentBuffer.filename != nullptr)
	{
		currentBuffer.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			auto& threadResources = *static_cast<ThreadResources*>(tr);
			auto& globalResources = *static_cast<GlobalResources*>(gr);
			auto& currentBuffer = static_cast<Buffer&>(request);
			auto& music = *currentBuffer.music;
			std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
			assert(bytesToCopy != 0u);
			currentBuffer.filename = music.files[music.previousTrack];
			currentBuffer.start = music.filePosition;
			currentBuffer.end = music.filePosition + bytesToCopy;
			globalResources.asynchronousFileManager.readFile(&currentBuffer, threadResources, globalResources);
		};
		globalResources.asynchronousFileManager.discard(&currentBuffer, threadResources, globalResources);
		return;
	}

	std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
	assert(bytesToCopy != 0u);
	currentBuffer.filename = music.files[music.previousTrack];
	currentBuffer.start = music.filePosition;
	currentBuffer.end = music.filePosition + bytesToCopy;
	globalResources.asynchronousFileManager.readFile(&currentBuffer, threadResources, globalResources);
}

void AmbientMusic::loadSoundDataPart2(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
	assert(bytesToCopy != 0u);
	auto& currentBuffer = *music.currentBuffer;
	currentBuffer.start = music.filePosition;
	currentBuffer.end = music.filePosition + bytesToCopy;
	currentBuffer.file = music.file;
	currentBuffer.filename = music.files[music.previousTrack];
	currentBuffer.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* data)
	{
		auto& threadResources = *static_cast<ThreadResources*>(tr);
		auto& globalResources = *static_cast<GlobalResources*>(gr);
		auto& currentBuffer = static_cast<Buffer&>(request);
		auto& music = *currentBuffer.music;

		std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
		currentBuffer.data = data;

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
	globalResources.asynchronousFileManager.readFile(&currentBuffer, threadResources, globalResources);
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
	unsigned int oldBufferLoadingAndIsFirstBuffer = bufferLoadingAndIsFirstBuffer.load(std::memory_order::memory_order_relaxed);
	unsigned int newBufferLoadingAndIsFirstBuffer = oldBufferLoadingAndIsFirstBuffer ^ 3u;
	bufferLoadingAndIsFirstBuffer.store(newBufferLoadingAndIsFirstBuffer, std::memory_order::memory_order_relaxed);
	currentBuffer = &bufferDescriptors[!(newBufferLoadingAndIsFirstBuffer & 1u)];

	std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, bytesRemaining);
	assert(bytesToCopy != 0u);
	currentBuffer->start = filePosition;
	currentBuffer->end = filePosition + bytesToCopy;
	currentBuffer->file = file;
	assert(currentBuffer->filename == nullptr);
	currentBuffer->filename = files[previousTrack];
	currentBuffer->fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* data)
	{
		auto& threadResources = *static_cast<ThreadResources*>(tr);
		auto& globalResources = *static_cast<GlobalResources*>(gr);
		auto& currentBuffer = static_cast<Buffer&>(request);
		auto& music = *currentBuffer.music;

		std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
		currentBuffer.data = data;

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
	globalResources.asynchronousFileManager.readFile(currentBuffer, threadResources, globalResources);
}

void AmbientMusic::onFirstBufferFinishedLoading(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
{
	onSoundDataLoadingFinished(music, threadResources, globalResources);
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