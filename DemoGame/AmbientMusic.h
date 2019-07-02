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

	enum class Action
	{
		load,
		unload
	};

	class Message : public SinglyLinked
	{
		friend class AmbientMusic;
		Action action;
	};
public:
	class StopRequest : public Message
	{
		friend class AmbientMusic;
		constexpr static unsigned int numberOfComponentsToUnload = numberOfBuffers;
		unsigned int numberOfComponentsUnloaded = 0u;
		void(*callback)(StopRequest& request, void* tr, void* gr);
	public:
		StopRequest(void(*callback1)(StopRequest& request, void* tr, void* gr)) : callback(callback1)
		{}
	};
private:
	
	struct Buffer : public AsynchronousFileManager::ReadRequest, public Message
	{
		using Message::action;
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
	void findNextMusic(ThreadResources& threadResources, AsynchronousFileManager& asynchronousFileManager, void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources));
	void fillAndSubmitBuffer(Buffer& buffer, AsynchronousFileManager& asynchronousFileManager);
	static void submitBuffer(IXAudio2SourceVoice* musicPlayer, void* context, const unsigned char* data, std::size_t length);
	void addMessage(Message& request, ThreadResources& threadResources, GlobalResources&);
	bool processMessage(SinglyLinked*& temp, ThreadResources& threadResources, GlobalResources& globalResources);
	void continueRunning(ThreadResources& threadResources, GlobalResources& globalResources);
public:
	AmbientMusic(GlobalResources& sharedResources, const wchar_t* const * files, std::size_t fileCount);
	~AmbientMusic();
	void start(ThreadResources& executor, AsynchronousFileManager& asynchronousFileManager);
	void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources& globalResources);
};