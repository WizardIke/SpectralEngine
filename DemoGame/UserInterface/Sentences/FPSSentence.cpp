#include "FPSSentence.h"

FPSSentence::FPSSentence(ID3D12Device* const Device, Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size) :
	BaseSentence(10u, Device, Font, L"Fps 0", Position, Size, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
	timePassed(0u),
	count(0u) {}

FPSSentence::~FPSSentence() {}

void FPSSentence::update(uint32_t FrameIndex, float frameTime)
{
	timePassed += frameTime;
	++count;
	if (timePassed >= 1.f)
	{
		
		if (count > 99999)
		{
			text = L"Fps >99999";
		}
		else
		{
			text = L"Fps ";
			text += std::to_wstring(count);
		}
		
		if (count >= 60) color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
		else if (count >= 30) color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
		else color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

		count = 0;
		timePassed = 0.f;
	}
	BaseSentence::update(FrameIndex);
}