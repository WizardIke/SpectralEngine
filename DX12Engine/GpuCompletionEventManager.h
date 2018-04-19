#pragma once
#include "ResizingArray.h"
#include "frameBufferCount.h"
#include "Job.h"
#include <cassert>
#include "Array.h"
class BaseExecutor;
class SharedResources;

class GpuCompletionEventManager
{
	Array<ResizingArray<Job>, frameBufferCount> requests;
public:
	void update(BaseExecutor* const executor, SharedResources& sharedResources);
	void addRequest(void* requester, void(*unloadCallback)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources), uint32_t frameIndex);
};