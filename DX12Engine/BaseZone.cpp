#include "BaseZone.h"
#include "SharedResources.h"
#include "BaseExecutor.h"

BaseZone::~BaseZone() {}

void BaseZone::loadHighDetail(BaseExecutor* const executor, SharedResources& sharedResources)
{
	loadingMutex.lock();
	nextLevelOfDetail = high;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != high && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadHighDetail(this, executor, sharedResources);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::loadMediumDetail(BaseExecutor* const executor, SharedResources& sharedResources)
{
	loadingMutex.lock();
	nextLevelOfDetail = medium;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != medium && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadMediumDetail(this, executor, sharedResources);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::loadLowDetail(BaseExecutor* const executor, SharedResources& sharedResources)
{
	loadingMutex.lock();
	nextLevelOfDetail = low;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != low && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		vTable->loadLowDetail(this, executor, sharedResources);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::unloadAll(BaseExecutor* const executor, SharedResources& sharedResources)
{
	loadingMutex.lock();
	nextLevelOfDetail = unloaded;
	auto oldLod = levelOfDetail.load(std::memory_order::memory_order_acquire);
	if (oldLod != unloaded && !transitioning)
	{
		transitioning = true;
		loadingMutex.unlock();
		if (oldLod == high) unloadHighDetail(executor, sharedResources);
		else if (oldLod == medium) unloadMediumDetail(executor, sharedResources);
		else unloadLowDetail(executor, sharedResources);
		return;
	}
	loadingMutex.unlock();
}

void BaseZone::unloadHighDetail(BaseExecutor* executor, SharedResources& sharedResources)
{
	executor->gpuCompletionEventManager.addRequest(this, vTable->unloadHighDetail, sharedResources.graphicsEngine.frameIndex);
}

void BaseZone::unloadMediumDetail(BaseExecutor* executor, SharedResources& sharedResources)
{
	executor->gpuCompletionEventManager.addRequest(this, vTable->unloadMediumDetail, sharedResources.graphicsEngine.frameIndex);
}

void BaseZone::unloadLowDetail(BaseExecutor* executor, SharedResources& sharedResources)
{
	executor->gpuCompletionEventManager.addRequest(this, vTable->unloadLowDetail, sharedResources.graphicsEngine.frameIndex);
}