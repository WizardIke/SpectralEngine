#pragma once
#include <vector>
#include "frameBufferCount.h"
#include "Job.h"
class BaseExecutor;

class GpuCompletionEventManager
{
	std::vector<Job> requests[frameBufferCount];
public:
	void update(BaseExecutor* const executor);
	void addRequest(void* requester, void(*unloadCallback)(void* const requester, BaseExecutor* const executor), uint32_t frameIndex);
};