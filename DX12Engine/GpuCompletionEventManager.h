#pragma once
#include <cstddef>
#include "ResizingArray.h"
#include "Delegate.h"
#include <array>

template<std::size_t bufferCount>
class GpuCompletionEventManager
{
	std::array<ResizingArray<Delegate<void(void* tr)>>, bufferCount> requests;
public:
	void update(std::size_t bufferIndex, void* tr)
	{
		for (auto& request : requests[bufferIndex])
		{
			request(tr);
		}
		requests[bufferIndex].clear();
	}
	
	void addRequest(void* requester, void(*unloadCallback)(void* const requester, void* tr), std::size_t bufferIndex)
	{
		requests[bufferIndex].emplace_back(requester, unloadCallback);
	}
};