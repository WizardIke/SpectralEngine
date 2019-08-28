#pragma once
#include <DynamicTextRenderer.h>

class CPUUsageSentence : public DynamicTextRenderer
{
	unsigned long long oldTotalTime;
	unsigned long long oldIdleTime;
public:
	CPUUsageSentence(ID3D12Device* Device, DirectX::XMFLOAT2 Position, DirectX::XMFLOAT2 Size, DirectX::XMFLOAT4 color);
	CPUUsageSentence(const CPUUsageSentence&) = default;
	~CPUUsageSentence() = default;
	void update();
};