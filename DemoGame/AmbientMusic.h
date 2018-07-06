#pragma once
#include <XAudio2SourceVoice.h>
#include <File.h>
#include <atomic>
#include <stdint.h>
class ThreadResources;
class GlobalResources;

class AmbientMusic : private IXAudio2VoiceCallback
{
	//The total size of front plus back buffer
	constexpr static size_t rawSoundDataBufferSize = 1310720u; //3364.6 ms of data

	alignas(4u) uint8_t rawSoundData[rawSoundDataBufferSize];
	XAudio2SourceVoice musicPlayer;
	std::atomic<unsigned int> bufferLoadingAndIsFirstBuffer = 0u;
	const wchar_t* const * files;
	size_t fileCount;
	size_t previousTrack;
	File file;
	size_t filePosition;
	size_t bytesRemaining;
	size_t bytesNeeded;
	uint8_t* currentBuffer;
	void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);

	virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnStreamEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;

	void update(ThreadResources& executor, GlobalResources& sharedResources);
	void findNextMusic(ThreadResources& threadResources, GlobalResources& globalResources, void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources));
	static void loadSoundData(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
	static void onSoundDataLoadingFinished(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
public:
	AmbientMusic(ThreadResources& executor, GlobalResources& sharedResources, const wchar_t* const * files, size_t fileCount);
	~AmbientMusic();
	void start();
};