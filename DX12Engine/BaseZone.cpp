#include "BaseZone.h"
#include "SharedResources.h"
#include "BaseExecutor.h"

BaseZone::~BaseZone() {}

void BaseZone::loadHighDetail(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = high;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != high && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadHighDetail(this, executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::loadMediumDetail(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = medium;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != medium && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadMediumDetail(this, executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::loadLowDetail(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = low;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != low && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadLowDetail(this, executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::unloadAll(BaseExecutor* const executor)
{
	loadingMutex.lock();
	nextLevelOfDetail = unloaded;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != unloaded && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		if (oldLod == high) unloadHighDetail(executor);
		else if (oldLod == medium) unloadMediumDetail(executor);
		else unloadLowDetail(executor);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::unloadHighDetail(BaseExecutor* executor)
{
	executor->vRamFreeingManager.addRequest(this, vTable->unloadHighDetail, executor->sharedResources->graphicsEngine.frameIndex);
}

void BaseZone::unloadMediumDetail(BaseExecutor* executor)
{
	executor->vRamFreeingManager.addRequest(this, vTable->unloadMediumDetail, executor->sharedResources->graphicsEngine.frameIndex);
}

void BaseZone::unloadLowDetail(BaseExecutor* executor)
{
	executor->vRamFreeingManager.addRequest(this, vTable->unloadLowDetail, executor->sharedResources->graphicsEngine.frameIndex);
}