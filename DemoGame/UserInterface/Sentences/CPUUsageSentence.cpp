#include "CPUUsageSentence.h"
#include <BaseExecutor.h>
#include <SharedResources.h>

CPUUsageSentence::CPUUsageSentence(ID3D12Device* const Device, Font* const Font, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size, const DirectX::XMFLOAT4 color) : 
	BaseSentence(8u, Device, Font, L"Cpu 0%", Position, Size, color)
{
	queryHandle = nullptr;

	PDH_STATUS status;

	try
	{
		// Create a query object to poll cpu usage.
		status = PdhOpenQueryW(nullptr, 0, &queryHandle);
		if (status != ERROR_SUCCESS) {throw false;}

		// Set query object to poll all cpus in the system.
		status = PdhAddEnglishCounterW(queryHandle, L"\\Processor(_Total)\\% processor time", 0u, &counterHandle);
		if (status != ERROR_SUCCESS) {throw false;}
	}
	catch (...)
	{
		if (queryHandle) PdhCloseQuery(queryHandle);
		throw;
	}
}

CPUUsageSentence::~CPUUsageSentence()
{
	if(queryHandle) PdhCloseQuery(queryHandle);
}

void CPUUsageSentence::update(BaseExecutor* const executor)
{
	PDH_FMT_COUNTERVALUE value;

	timeSinceLastUpdate += executor->sharedResources->timer.frameTime;
	if (timeSinceLastUpdate > 1.0f)
	{
		timeSinceLastUpdate = 0.0f;

		PdhCollectQueryData(queryHandle);

		PdhGetFormattedCounterValue(counterHandle, PDH_FMT_LONG, nullptr, &value);

		text = L"Cpu ";
		text += std::to_wstring(value.longValue);
		text += L"%";
	}
	BaseSentence::update(executor->sharedResources->graphicsEngine.frameIndex);
}