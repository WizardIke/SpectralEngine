#pragma once
#include <cstddef>
#include "ResizingArray.h"
#include "Delegate.h"
#include "Array.h"

template<std::size_t bufferCount>
class GpuCompletionEventManager
{
	Array<ResizingArray<Delegate<void(void* executor, void* sharedResources)>>, bufferCount> requests;
public:
	void update(std::size_t bufferIndex, void* executor, void* sharedResources)
	{
		for (auto& request : requests[bufferIndex])
		{
			request(executor, sharedResources);
		}
		requests[bufferIndex].clear();
	}
	
	void addRequest(void* requester, void(*unloadCallback)(void* const requester, void* executor, void* sharedResources), std::size_t bufferIndex)
	{
		requests[bufferIndex].emplace_back(requester, unloadCallback);
	}
};