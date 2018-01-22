#include "GpuCompletionEventManager.h"
#include "BaseExecutor.h"
#include "SharedResources.h"

void GpuCompletionEventManager::update(BaseExecutor* const executor)
{
	auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
	for (Job& request : requests[frameIndex])
	{
		request(executor);
	}
	requests[frameIndex].clear();
}

void GpuCompletionEventManager::addRequest(void* requester, void(*unloadCallback)(void* const requester, BaseExecutor* const executor), uint32_t frameIndex)
{
	requests[frameIndex].push_back({ requester, unloadCallback });
}