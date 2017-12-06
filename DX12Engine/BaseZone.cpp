#include "BaseZone.h"
#include "SharedResources.h"
#include "BaseExecutor.h"

BaseZone::~BaseZone() {}

void BaseZone::loadHighDetail(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = high;
	if (levelOfDetail != high && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadHighDetailJobs(this, executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::loadMediumDetail(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = medium;
	if (levelOfDetail != medium && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadMediumDetailJobs(this, executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::loadLowDetail(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = low;
	if (levelOfDetail != low && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadLowDetailJobs(this, executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::unloadAll(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = unloaded;
	if (levelOfDetail != unloaded && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		if (levelOfDetail == high) unloadHighDetail(executor);
		else if (levelOfDetail == medium) unloadMediumDetail(executor);
		else unloadLowDetail(executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::unloadMediumDetail(BaseExecutor* executor)
{
	executor->vRamFreeingManager.addRequest(this, [](void* const requester, BaseExecutor* const executor)
	{
		BaseZone* const zone = reinterpret_cast<BaseZone* const>(requester);
		zone->vTable->unloadMediumDetailJobs(zone, executor);
	}, executor->sharedResources->graphicsEngine.frameIndex);
}

void BaseZone::unloadLowDetail(BaseExecutor* executor)
{
	executor->vRamFreeingManager.addRequest(this, [](void* const requester, BaseExecutor* const executor)
	{
		BaseZone* const zone = reinterpret_cast<BaseZone* const>(requester);
		zone->vTable->unloadLowDetailJobs(zone, executor);
	}, executor->sharedResources->graphicsEngine.frameIndex);
}



void BaseZone::unloadHighDetail(BaseExecutor* executor)
{
	executor->vRamFreeingManager.addRequest(this, [](void* const requester, BaseExecutor* const executor)
	{
		BaseZone* const zone = reinterpret_cast<BaseZone* const>(requester);
		zone->vTable->unloadHighDetailJobs(zone, executor);
	}, executor->sharedResources->graphicsEngine.frameIndex);
}