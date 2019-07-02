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
	ioCompletionQueue(sharedResources.ioCompletionQueue),
	stopRequest(nullptr)
{
	for(std::size_t i = 0u; i != numberOfBuffers; ++i)
	{
		bufferDescriptors[i].music = this;

		bufferDescriptors[i].fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, void* gr, const unsigned char* data)
		{
			auto& threadResources = *static_cast<ThreadResources*>(tr);
			auto& globalResources = *static_cast<GlobalResources*>(gr);
			auto& buffer = static_cast<Buffer&>(request);
			auto& music = *buffer.music;
			buffer.data = data;

			std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, music.bytesRemaining);
			music.bytesRemaining -= bytesToCopy;
			music.filePosition += bytesToCopy;

			if(music.bytesRemaining != 0u)
			{
				submitBuffer(music.musicPlayer, &buffer, buffer.data, buffer.end - buffer.start);
				music.continueRunning(threadResources, globalResources);
			}
			else
			{
				music.currentBuffer = &buffer;
				music.file.close();
				music.findNextMusic(threadResources, asynchronousFileManager, [](AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
				{
					auto& buffer = *music.currentBuffer;
					submitBuffer(music.musicPlayer, &buffer, buffer.data, buffer.end - buffer.start);
					music.continueRunning(threadResources, globalResources);
				});
			}
		};

		bufferDescriptors[i].deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			auto& threadResources = *static_cast<ThreadResources*>(tr);
			auto& globalResources = *static_cast<GlobalResources*>(gr);
			auto& buffer = static_cast<Buffer&>(request);
			auto& music = *buffer.music;

			buffer.action = Action::load;
			music.addMessage(buffer, threadResources, globalResources);
		};
	}

	infoRequest.music = this;
	infoRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, void*, const unsigned char* data)
	{
		const SoundDecoder::UncompressedFileInfo& headerInfo = *reinterpret_cast<const SoundDecoder::UncompressedFileInfo*>(data);
		auto& music = *static_cast<InfoRequest&>(request).music;
		music.bytesRemaining = headerInfo.dataSize;
		asynchronousFileManager.discard(request);
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

AmbientMusic::~AmbientMusic() {}

void AmbientMusic::OnBufferEnd(void* pBufferContext)
{
	auto& buffer = *static_cast<Buffer*>(pBufferContext);
	IOCompletionPacket updateTask;
	updateTask.numberOfBytesTransfered = 0u;
	updateTask.overlapped = static_cast<LPOVERLAPPED>(&buffer);
	updateTask.completionKey = reinterpret_cast<ULONG_PTR>(static_cast<bool(*)(void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)>(
		[](void*, void* gr, DWORD, LPOVERLAPPED overlapped)
	{
		auto& globalResources = *static_cast<GlobalResources*>(gr);
		globalResources.asynchronousFileManager.discard(*static_cast<Buffer*>(overlapped));
		return true;
	}));
	ioCompletionQueue.push(updateTask);
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

void AmbientMusic::start(ThreadResources& threadResources, AsynchronousFileManager& asynchronousFileManager)
{
	for(std::size_t i = 0u; i != numberOfBuffers; ++i)
	{
		bufferDescriptors[i].action = Action::load;
		freeBufferDescriptors.push(static_cast<Message*>(&bufferDescriptors[i]));
	}

	findNextMusic(threadResources, asynchronousFileManager, [](AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		music.run(threadResources, globalResources);
		music.musicPlayer->Start(0u, 0u);
	});
}

void AmbientMusic::addMessage(Message& request, ThreadResources& threadResources, GlobalResources&)
{
	bool needsStarting = freeBufferDescriptors.push(&request);
	if(needsStarting)
	{
		threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& music = *static_cast<AmbientMusic*>(requester);
			music.run(threadResources, globalResources);
		}});
	}
}

void AmbientMusic::stop(StopRequest& stopRequest1, ThreadResources& threadResources, GlobalResources& globalResources)
{
	stopRequest1.action = Action::unload;
	addMessage(stopRequest1, threadResources, globalResources);
}

void AmbientMusic::run(ThreadResources& threadResources, GlobalResources& globalResources)
{
	do
	{
		SinglyLinked* temp = freeBufferDescriptors.popAll();
		do
		{
			auto pause = processMessage(temp, threadResources, globalResources);
			if(pause) return;
		} while(temp != nullptr);
	} while(!freeBufferDescriptors.stop());
}

void AmbientMusic::continueRunning(ThreadResources& threadResources, GlobalResources& globalResources)
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
			auto pause = processMessage(temp, threadResources, globalResources);
			if(pause) return;
		} while(temp != nullptr);
		if(freeBufferDescriptors.stop()) return;
		temp = freeBufferDescriptors.popAll();
	}
}

bool AmbientMusic::processMessage(SinglyLinked*& temp, ThreadResources& threadResources, GlobalResources& globalResources)
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
				file.close();
				stopRequest->callback(*stopRequest, &threadResources, &globalResources);
				freeBufferDescriptors.stop();
				return true;
			}
		}
		else
		{
			pendingMessages = temp;
			fillAndSubmitBuffer(static_cast<Buffer&>(message), globalResources.asynchronousFileManager);
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

void AmbientMusic::findNextMusic(ThreadResources& threadResources, AsynchronousFileManager& asynchronousFileManager, void(*callbackFunc)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources))
{
	callback = callbackFunc;

	std::uniform_int_distribution<unsigned long long> distribution(0, fileCount - 1u);
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
	filePosition = 4 * 1024u;

	auto& request = infoRequest;
	request.filename = files[track];
	request.file = asynchronousFileManager.openFileForReading(request.filename);
	file = request.file;
	asynchronousFileManager.readFile(request);
}

void AmbientMusic::fillAndSubmitBuffer(Buffer& buffer, AsynchronousFileManager& asynchronousFileManager)
{
	std::size_t bytesToCopy = std::min(rawSoundDataBufferSize, bytesRemaining);
	assert(bytesToCopy != 0u);
	buffer.filename = files[previousTrack];
	buffer.file = file;
	buffer.start = filePosition;
	buffer.end = filePosition + bytesToCopy;
	asynchronousFileManager.readFile(buffer);
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