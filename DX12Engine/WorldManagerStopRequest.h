#pragma once
#include <atomic>

class WorldManagerStopRequest
{
public:
	std::atomic<unsigned long> numberOfComponentsUnloaded = 0u;
	unsigned long numberOfComponentsToUnload;

	WorldManagerStopRequest** stopRequest;
	void(*callback)(WorldManagerStopRequest& stopRequest, void* tr);

	void onZoneStopped(void* tr)
	{
		if(numberOfComponentsUnloaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponentsToUnload - 1u))
		{
			callback(*this, tr);
		}
	}

	WorldManagerStopRequest(void(*callback1)(WorldManagerStopRequest& stopRequest, void* tr)) : callback(callback1) {}
};