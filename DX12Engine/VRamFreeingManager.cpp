#include "VRamFreeingManager.h"
#include "BaseExecutor.h"
#include "SharedResources.h"

void VRamFreeingManager::update(BaseExecutor* const executor)
{
	auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
	for (Request& request : requests[frameIndex])
	{
		request.unload(request.requester, executor);
	}
	requests[frameIndex].clear();
}

void VRamFreeingManager::addRequest(void* requester, void(*unloadCallback)(void* const requester, BaseExecutor* const executor), uint32_t frameIndex)
{
	requests[frameIndex].push_back({ requester, unloadCallback });
}