#pragma once

#include <DynamicTextRenderer.h>

class FPSSentence : public DynamicTextRenderer
{
	float timePassed;
	unsigned int count;
public:
	FPSSentence(ID3D12Device* Device, const Font* Font, DirectX::XMFLOAT2 Position, DirectX::XMFLOAT2 Size);
	~FPSSentence() = default;
	void update(float frameTime);
};