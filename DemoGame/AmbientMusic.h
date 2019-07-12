#pragma once
#include <XAudio2SourceVoice.h>
#include <File.h>
#include <cstdint>
#include <AsynchronousFileManager.h>
#include <ActorQueue.h>
class ThreadResources;
class SoundEngine;

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
		void(*callback)(StopRequest& request, void* tr);
	public:
		StopRequest(void(*callback1)(StopRequest& request, void* tr)) : callback(callback1)
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
	AsynchronousFileManager& asynchronousFileManager;
	SinglyLinked* pendingMessages;
	Buffer bufferDescriptors[numberOfBuffers];
	InfoRequest infoRequest;
	void(*callback)(AmbientMusic& music, ThreadResources& threadResources);
	StopRequest* stopRequest;

	virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnStreamEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;

	void run(ThreadResources& threadResources);
	void findNextMusic(ThreadResources& threadResources, void(*callback)(AmbientMusic& music, ThreadResources& threadResources));
	void fillAndSubmitBuffer(Buffer& buffer);
	static void submitBuffer(IXAudio2SourceVoice* musicPlayer, void* context, const unsigned char* data, std::size_t length);
	void addMessage(Message& request, ThreadResources& threadResources);
	bool processMessage(SinglyLinked*& temp, ThreadResources& threadResources);
	void continueRunning(ThreadResources& threadResources);
public:
	AmbientMusic(SoundEngine& soundEngine, AsynchronousFileManager& asynchronousFileManager, const wchar_t* const * files, std::size_t fileCount);
	~AmbientMusic();
	void start(ThreadResources& threadResources);
	void stop(StopRequest& stopRequest, ThreadResources& threadResources);
};