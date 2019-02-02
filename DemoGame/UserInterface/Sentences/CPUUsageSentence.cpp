#include "CPUUsageSentence.h"
#include "../../GlobalResources.h"

CPUUsageSentence::CPUUsageSentence(ID3D12Device* const Device, Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size, const DirectX::XMFLOAT4 color) : 
	BaseSentence(8u, Device, Font, L"Cpu 0%", Position, Size, color)
{
	// Create a query object to poll cpu usage.
	queryHandle = nullptr;
	PDH_STATUS status = PdhOpenQueryW(nullptr, 0, &queryHandle);
	if(status != ERROR_SUCCESS) { throw false; }

	// Set query object to poll all cpus in the system.
	status = PdhAddEnglishCounterW(queryHandle, L"\\Processor(_Total)\\% processor time", 0u, &counterHandle);
	if(status != ERROR_SUCCESS)
	{
		PdhCloseQuery(queryHandle);
		throw false;
	}
}

CPUUsageSentence::~CPUUsageSentence()
{
	if(queryHandle) PdhCloseQuery(queryHandle);
}

void CPUUsageSentence::update(float frameTime)
{
	timeSinceLastUpdate += frameTime;
	if (timeSinceLastUpdate > 1.0f)
	{
		timeSinceLastUpdate = 0.0f;

		PdhCollectQueryData(queryHandle);

		PDH_FMT_COUNTERVALUE value;
		PdhGetFormattedCounterValue(counterHandle, PDH_FMT_LONG, nullptr, &value);

		text = L"Cpu ";
		text += std::to_wstring(value.longValue);
		text += L"%";
	}
}