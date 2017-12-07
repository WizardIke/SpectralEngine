#include "RenderCountSentence.h"

DrawCallCountSentence::DrawCallCountSentence(ID3D12Device* const Device, Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size) : 
	BaseSentence(21u, Device, Font, L"Draw calls 0", Position, Size, DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)) {}

DrawCallCountSentence::~DrawCallCountSentence() {}

void DrawCallCountSentence::update(unsigned int RenderCount, uint32_t FrameIndex)
{
	text = L"Draw calls ";
	text += std::to_wstring(RenderCount);

	BaseSentence::update(FrameIndex);
}