#pragma once
#include <XAudio2SourceVoice.h>
#include <memory>
#include <stdio.h>
class BaseExecutor;

class AmbientMusic : IXAudio2VoiceCallback
{
	constexpr static size_t rawSoundDataBufferSize = 17640u; //100 ms of data

	XAudio2SourceVoice musicPlayer;
	unsigned long long previousTrack;
	FILE* currentFile;
	size_t bytesRemaining;
	std::unique_ptr<uint8_t[]> rawSoundData;
	bool firstBuffer = true;

	virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnStreamEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;

	void update(BaseExecutor* const executor);
public:
	AmbientMusic(BaseExecutor* const executor);
	~AmbientMusic() {}
};