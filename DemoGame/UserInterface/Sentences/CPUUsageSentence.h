#pragma once

#pragma comment(lib, "pdh.lib")

#include <pdh.h>
#include <BaseSentence.h>

class CPUUsageSentence : public BaseSentence
{
	float timeSinceLastUpdate = 0.0f;
	HQUERY queryHandle;
	HCOUNTER counterHandle;
public:
	CPUUsageSentence(ID3D12Device* const Device, Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size, const DirectX::XMFLOAT4 color);
	~CPUUsageSentence();
	void update(BaseExecutor* const executor);
};