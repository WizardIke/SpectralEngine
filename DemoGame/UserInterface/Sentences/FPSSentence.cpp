#include "FPSSentence.h"

FPSSentence::FPSSentence(ID3D12Device* const Device, const Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size) :
	DynamicTextRenderer(10u, Device, Font, L"Fps 0", Position, Size, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
	timePassed(0u),
	count(0u)
{}

void FPSSentence::update(float frameTime)
{
	timePassed += frameTime;
	++count;
	if (timePassed >= 1.f)
	{
		unsigned int fps = (unsigned int)(count / timePassed);
		count = 0;
		timePassed = 0.f;

		if (fps > 99999)
		{
			text = L"Fps >99999";
		}
		else
		{
			text = L"Fps ";
			text += std::to_wstring(fps);
		}
		
		if (fps >= 60) color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
		else if (fps >= 30) color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
		else color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	}
}