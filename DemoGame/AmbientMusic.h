#pragma once
#include <XAudio2SourceVoice.h>
#include <File.h>
#include <cstdint>
#include <AsynchronousFileManager.h>
#include <ActorQueue.h>
#include <IOCompletionQueue.h>
class ThreadResources;
class GlobalResources;

class AmbientMusic : private IXAudio2VoiceCallback
{
	constexpr static std::size_t numberOfBuffers = 3u;
public:
	class StopRequest : public AsynchronousFileManager::ReadRequest
	{
	public:
		constexpr static unsigned int numberOfComponentsToUnload = numberOfBuffers;
		unsigned int numberOfComponentsUnloaded = 0u;

		StopRequest(void(*callback)(ReadRequest& request, void* tr, void* gr))
		{
			deleteReadRequest = callback;
		}
	};
private:
	struct Buffer : public AsynchronousFileManager::ReadRequest
	{
		const unsigned char* data;
		AmbientMusic* music;
	};
	struct InfoRequest : public AsynchronousFileManager::ReadRequest
	{
		AmbientMusic* music;
	};
	//The size of one of the numberOfBuffers sound buffers
	constexpr static std::size_t rawSoundDataBufferSize = 64u * 1024u;
	XAudio2SourceVoice musicPlayer;
	const wchar_t* const * files;
	std::size_t fileCount;
	File file;
	std::size_t previousTrack;
	std::size_t filePosition;
	std::size_t bytesRemaining;
	ActorQueue freeBufferDescriptors;
	Buffer* currentBuffer;
	IOCompletionQueue& ioCompletionQueue;
	SinglyLinked* pendingMessages;
	Buffer bufferDescriptors[numberOfBuffers];
	InfoRequest infoRequest;
	void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
	StopRequest* stopRequest;

	virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnStreamEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;

	void run(ThreadResources& threadResources, GlobalResources& globalResources);
	void findNextMusic(ThreadResources& threadResources, GlobalResources& globalResources, void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources));
	void fillAndSubmitBuffer(Buffer& buffer, ThreadResources& threadResources, GlobalResources& globalResources);
	static void submitBuffer(IXAudio2SourceVoice* musicPlayer, void* context, const unsigned char* data, std::size_t length);
	void addMessage(AsynchronousFileManager::ReadRequest& request, ThreadResources& threadResources, GlobalResources&);
	bool processMessage(SinglyLinked*& temp, ThreadResources& threadResources, GlobalResources& globalResources);
	void continueRunning(ThreadResources& threadResources, GlobalResources& globalResources);
public:
	AmbientMusic(GlobalResources& sharedResources, const wchar_t* const * files, std::size_t fileCount);
	~AmbientMusic();
	void start(ThreadResources& executor, GlobalResources& sharedResources);
	void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources& globalResources);
};