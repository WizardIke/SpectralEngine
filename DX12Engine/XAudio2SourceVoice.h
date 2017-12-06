#pragma once
#include <xaudio2.h>
#include "HresultException.h"
#include <stdint.h>

class XAudio2SourceVoice
{
	IXAudio2SourceVoice* data;
public:
	XAudio2SourceVoice(IXAudio2* audioDevice, const WAVEFORMATEX& format, uint32_t flags = 0u, float maxFrequencyRatio = 2.0f, IXAudio2VoiceCallback* callback = nullptr, const XAUDIO2_VOICE_SENDS* sendList = nullptr,
		const XAUDIO2_EFFECT_CHAIN* effectChain = nullptr)
	{
		HRESULT hr = audioDevice->CreateSourceVoice(&data, &format, flags, maxFrequencyRatio, callback, sendList, effectChain);
		if (FAILED(hr)) throw HresultException(hr);
	}

	IXAudio2SourceVoice* operator->() { return data; }

	operator IXAudio2SourceVoice*() { return data; }
	operator void*() { return data; }

	~XAudio2SourceVoice()
	{
		if (data) data->DestroyVoice();
	}
};