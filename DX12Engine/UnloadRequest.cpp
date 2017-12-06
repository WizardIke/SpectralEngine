#include "UnloadRequest.h"
#include "BaseZoneClass.h"
#include "SharedResources.h"

UnloadRequest::UnloadRequest(bool primary, BaseZoneClass* const baseZoneClass, void(*unload)(void* const worker, BaseJobExeClass* const executor)) :
	baseZoneClass(baseZoneClass), unload(unload), primary(primary)
{
	baseZoneClass->sharedResources->syncMutex.lock();
	frameIndex = baseZoneClass->sharedResources->graphicsEngine.frameIndex;
	//m_FrameIndex will be correct or next frame from the one we want, so we might wait one frame too long but it still works
	fenceValue = baseZoneClass->sharedResources->graphicsEngine.fenceValues[frameIndex] + 1ull; //add one so we wait for the frame to finish
	baseZoneClass->sharedResources->syncMutex.unlock();
}

bool UnloadRequest::update(BaseJobExeClass* const executor)
{
	std::unique_lock<decltype(executor->sharedResources->syncMutex)> lock(executor->sharedResources->syncMutex);
	if (executor->sharedResources->graphicsEngine.directFences[frameIndex]->GetCompletedValue() == fenceValue)
	{
		lock.unlock();
		if (primary)
		{
			unload(baseZoneClass, executor);
		}
		else
		{
			executor->sharedResources->backgroundQueue.push(Job(baseZoneClass, unload));
		}
		return true;
	}
	lock.unlock();
	return false;
}