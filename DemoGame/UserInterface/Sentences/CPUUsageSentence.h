#pragma once

#pragma comment(lib, "pdh.lib")

#include <pdh.h>
#include <DynamicTextRenderer.h>

class CPUUsageSentence : public DynamicTextRenderer
{
	float timeSinceLastUpdate = 0.0f;
	HQUERY queryHandle;
	HCOUNTER counterHandle;
public:
	CPUUsageSentence(ID3D12Device* Device, DirectX::XMFLOAT2 Position, DirectX::XMFLOAT2 Size, DirectX::XMFLOAT4 color);
	~CPUUsageSentence();
	void update(float frameTime);
};