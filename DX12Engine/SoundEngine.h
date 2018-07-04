#pragma once
#include <xaudio2.h>
#include "HresultException.h"
#undef min
#undef max

class SoundEngine
{
public:
	IXAudio2* audioDevice;
	IXAudio2MasteringVoice* masterVoice;

	SoundEngine()
	{
		HRESULT hr = XAudio2Create(&audioDevice, 0, XAUDIO2_DEFAULT_PROCESSOR);
		if (FAILED(hr)) throw HresultException(hr);

		hr = audioDevice->CreateMasteringVoice(&masterVoice);
		if (FAILED(hr)) throw HresultException(hr);
	}
};