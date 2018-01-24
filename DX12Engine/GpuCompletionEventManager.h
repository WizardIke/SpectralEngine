#pragma once
#include <vector>
#include "frameBufferCount.h"
#include "Job.h"
class BaseExecutor;
class SharedResources;

class GpuCompletionEventManager
{
	std::vector<Job> requests[frameBufferCount];
public:
	void update(BaseExecutor* const executor, SharedResources& sharedResources);
	void addRequest(void* requester, void(*unloadCallback)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources), uint32_t frameIndex);
};