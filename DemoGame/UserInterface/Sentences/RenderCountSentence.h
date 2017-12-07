#pragma once

#include <BaseSentence.h>

class DrawCallCountSentence : public BaseSentence
{
public:
	DrawCallCountSentence(ID3D12Device* const Device, Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size);
	~DrawCallCountSentence();
	void update(unsigned int RenderCount, uint32_t FrameIndex);
};