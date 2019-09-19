#include "AmbientMusic.h"
#include <SoundDecoder.h>
#include "ThreadResources.h"
#include <SoundEngine.h>
#include <random>

AmbientMusic::AmbientMusic(SoundEngine& soundEngine, AsynchronousFileManager& asynchronousFileManager, const ResourceLocation* files, std::size_t fileCount) :
	musicPlayer(soundEngine.audioDevice, []()
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
	previousTrack(std::numeric_limits<decltype(previousTrack)>::max()),
	files(files),
	fileCount(fileCount),
	asynchronousFileManager(asynchronousFileManager),
	stopRequest(nullptr)
{
	for(std::size_t i = 0u; i != numberOfBuffers; ++i)
	{
		bufferDescriptors[i].music = this;

		bufferDescriptors[i].fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager&, void* tr, const unsigned char* data)
		{
			auto& threadResources = *static_cast<ThreadResources*>(tr);
			auto& buffer = static_cast<Buffer&>(request);
			auto& music = *buffer.music;
			buffer.data = data;

			std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
			music.bytesRemaining -= bytesToCopy;
			music.filePosition += bytesToCopy;

			if(music.bytesRemaining != 0u)
			{
				submitBuffer(music.musicPlayer, &buffer, buffer.data, static_cast<std::size_t>(buffer.end - buffer.start));
				music.continueRunning(threadResources);
			}
			else
			{
				music.currentBuffer = &buffer;
				music.findNextMusic(threadResources, [](AmbientMusic& music, ThreadResources& threadResources)
				{
					auto& buffer = *music.currentBuffer;
					submitBuffer(music.musicPlayer, &buffer, buffer.data, static_cast<std::size_t>(buffer.end - buffer.start));
					music.continueRunning(threadResources);
				});
			}
		};

		bufferDescriptors[i].deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr)
		{
			auto& threadResources = *static_cast<ThreadResources*>(tr);
			auto& buffer = static_cast<Buffer&>(request);
			auto& music = *buffer.music;

			buffer.action = Action::load;
			music.addMessage(buffer, threadResources);
		};
	}

	infoRequest.music = this;
	infoRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* data)
	{
		const SoundDecoder::UncompressedFileInfo& headerInfo = *reinterpret_cast<const SoundDecoder::UncompressedFileInfo*>(data);
		auto& music = *static_cast<InfoRequest&>(request).music;
		music.bytesRemaining = headerInfo.dataSize;
		asynchronousFileManager.discard(request);
	};
	infoRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr)
	{
		auto& music = *static_cast<InfoRequest&>(request).music;
		auto& threadResources = *static_cast<ThreadResources*>(tr);
		music.callback(music, threadResources);
	};
}

AmbientMusic::~AmbientMusic() {}

void AmbientMusic::OnBufferEnd(void* pBufferContext)
{
	auto& buffer = *static_cast<Buffer*>(pBufferContext);
	IOCompletionPacket updateTask;
	updateTask.numberOfBytesTransfered = 0u;
	updateTask.overlapped = static_cast<LPOVERLAPPED>(&buffer);
	updateTask.completionKey = reinterpret_cast<ULONG_PTR>(static_cast<bool(*)(void* tr, DWORD numberOfBytes, LPOVERLAPPED overlapped)>(
		[](void*, DWORD, LPOVERLAPPED overlapped)
	{
		Buffer& buffer = *static_cast<Buffer*>(overlapped);
		buffer.music->asynchronousFileManager.discard(*static_cast<Buffer*>(overlapped));
		return true;
	}));
	buffer.music->asynchronousFileManager.getIoCompletionQueue().push(updateTask);
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

void AmbientMusic::start(ThreadResources& threadResources)
{
	for(std::size_t i = 0u; i != numberOfBuffers; ++i)
	{
		bufferDescriptors[i].action = Action::load;
		freeBufferDescriptors.push(static_cast<Message*>(&bufferDescriptors[i]));
	}

	findNextMusic(threadResources, [](AmbientMusic& music, ThreadResources& threadResources)
	{
		music.run(threadResources);
		music.musicPlayer->Start(0u, 0u);
	});
}

void AmbientMusic::addMessage(Message& request, ThreadResources& threadResources)
{
	bool needsStarting = freeBufferDescriptors.push(&request);
	if(needsStarting)
	{
		threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources)
		{
			auto& music = *static_cast<AmbientMusic*>(requester);
			music.run(threadResources);
		}});
	}
}

void AmbientMusic::stop(StopRequest& stopRequest1, ThreadResources& threadResources)
{
	stopRequest1.action = Action::unload;
	addMessage(stopRequest1, threadResources);
}

void AmbientMusic::run(ThreadResources& threadResources)
{
	do
	{
		SinglyLinked* temp = freeBufferDescriptors.popAll();
		do
		{
			auto pause = processMessage(temp, threadResources);
			if(pause) return;
		} while(temp != nullptr);
	} while(!freeBufferDescriptors.stop());
}

void AmbientMusic::continueRunning(ThreadResources& threadResources)
{
	SinglyLinked* temp = pendingMessages;
	while(temp == nullptr)
	{
		if(freeBufferDescriptors.stop()) return;
		temp = freeBufferDescriptors.popAll();
	}
	while(true)
	{
		do
		{
			auto pause = processMessage(temp, threadResources);
			if(pause) return;
		} while(temp != nullptr);
		if(freeBufferDescriptors.stop()) return;
		temp = freeBufferDescriptors.popAll();
	}
}

bool AmbientMusic::processMessage(SinglyLinked*& temp, ThreadResources& threadResources)
{
	auto& message = *static_cast<Message*>(temp);
	temp = temp->next; //Allow reuse of next
	if(message.action == Action::load)
	{
		if(stopRequest != nullptr)
		{
			++stopRequest->numberOfComponentsUnloaded;
			if(stopRequest->numberOfComponentsUnloaded == StopRequest::numberOfComponentsToUnload)
			{
				stopRequest->callback(*stopRequest, &threadResources);
				freeBufferDescriptors.stop();
				return true;
			}
		}
		else
		{
			pendingMessages = temp;
			fillAndSubmitBuffer(static_cast<Buffer&>(message));
			return true;
		}
	}
	else
	{
		stopRequest = &static_cast<StopRequest&>(message);
		musicPlayer->Discontinuity();
	}
	return false;
}

void AmbientMusic::findNextMusic(ThreadResources& threadResources, void(*callbackFunc)(AmbientMusic& music, ThreadResources& threadResources))
{
	callback = callbackFunc;

	std::uniform_int_distribution<std::size_t> distribution(0, fileCount - 1u);
	std::size_t track = distribution(threadResources.randomNumberGenerator);

	if(track == previousTrack)
	{
		++track;
		if(track == fileCount)
		{
			track = 0u;
		}
	}
	previousTrack = track;
	filePosition = files[track].start + 4u * 1024u;

	auto& request = infoRequest;
	request.start = files[track].start;
	request.end = files[track].start + 4u * 1024u;
	asynchronousFileManager.read(request);
}

void AmbientMusic::fillAndSubmitBuffer(Buffer& buffer)
{
	std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, bytesRemaining);
	assert(bytesToCopy != 0u);
	buffer.start = filePosition;
	buffer.end = filePosition + bytesToCopy;
	asynchronousFileManager.read(buffer);
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
	if(FAILED(hr)) throw HresultException(hr);
}