#include "CPUUsageSentence.h"
#include <Window.h>
#include <string>

static unsigned long long fileTimeToInt64(FILETIME ft)
{
	ULARGE_INTEGER foo;
	foo.LowPart = ft.dwLowDateTime;
	foo.HighPart = ft.dwHighDateTime;
	return static_cast<unsigned long long>(foo.QuadPart);
}

CPUUsageSentence::CPUUsageSentence(ID3D12Device* const device, const DirectX::XMFLOAT2 position, const DirectX::XMFLOAT2 size, const DirectX::XMFLOAT4 color) : 
	DynamicTextRenderer(8u, device, L"Cpu 0%", position, size, color)
{
	FILETIME idleTime{}, kernalTime{}, userTime{};
	GetSystemTimes(&idleTime, &kernalTime, &userTime);
	oldTotalTime = fileTimeToInt64(kernalTime) + fileTimeToInt64(userTime);
	oldIdleTime = fileTimeToInt64(idleTime);
}

void CPUUsageSentence::update()
{
	FILETIME idleTime{}, kernalTime{}, userTime{};
	GetSystemTimes(&idleTime, &kernalTime, &userTime);

	auto newTotalTime = fileTimeToInt64(kernalTime) + fileTimeToInt64(userTime);
	auto newIdleTime = fileTimeToInt64(idleTime);
	auto totalTimePassed = newTotalTime - oldTotalTime;
	auto idleTimePassed = newIdleTime - oldIdleTime;

	auto getCpuPercentage = [](ULONGLONG totalTime, ULONGLONG idleTime)
	{
		if (totalTime == 0u)
		{
			return 0.0;
		}
		auto percentage = static_cast<double>(idleTime) / static_cast<double>(totalTime) * 100.0;
		if (percentage > 100.0)
		{
			percentage = 100.0;
		}
		return 100.0 - percentage;
	};
	auto percentage = getCpuPercentage(totalTimePassed, idleTimePassed);

	oldTotalTime += totalTimePassed / 128u;
	oldIdleTime += idleTimePassed / 128u;

	text = L"Cpu ";
	text += std::to_wstring(static_cast<unsigned short>(percentage));
	text += L"%";
}