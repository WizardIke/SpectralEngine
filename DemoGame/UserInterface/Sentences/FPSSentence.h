#pragma once

#include <BaseSentence.h>

class FPSSentence : public BaseSentence
{
	float timePassed;
	unsigned int count;
public:
	FPSSentence(ID3D12Device* const Device, Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size);
	~FPSSentence();
	void update(uint32_t FrameIndex, float frameTime);
};