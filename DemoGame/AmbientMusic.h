#pragma once
#include <XAudio2SourceVoice.h>
#include <memory>
#include <stdio.h>
class BaseExecutor;
class SharedResources;

class AmbientMusic : IXAudio2VoiceCallback
{
	//The total size of front plus back buffer
	constexpr static size_t rawSoundDataBufferSize = 352800u; //2000 ms of data

	XAudio2SourceVoice musicPlayer;
	unsigned long long previousTrack;
	FILE* currentFile;
	size_t bytesRemaining;
	uint8_t rawSoundData[rawSoundDataBufferSize];
	bool firstBuffer = true;

	virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnStreamEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;

	void update(BaseExecutor* const executor, SharedResources& sharedResources);
public:
	AmbientMusic(BaseExecutor* const executor, SharedResources& sharedResources);
	~AmbientMusic() {}
};