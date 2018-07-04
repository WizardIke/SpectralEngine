#pragma once
#include "ResizingArray.h"
#include "frameBufferCount.h"
#include "Delegate.h"
#include "Array.h"

class GpuCompletionEventManager
{
	Array<ResizingArray<Delegate<void(void* executor, void* sharedResources)>>, frameBufferCount> requests;
public:
	void update(uint32_t frameIndex, void* executor, void* sharedResources)
	{
		for (auto& request : requests[frameIndex])
		{
			request(executor, sharedResources);
		}
		requests[frameIndex].clear();
	}
	
	void addRequest(void* requester, void(*unloadCallback)(void* const requester, void* executor, void* sharedResources), uint32_t frameIndex)
	{
		requests[frameIndex].push_back({ requester, unloadCallback });
	}
};