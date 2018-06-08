#include "BaseZone.h"
#include "SharedResources.h"
#include "BaseExecutor.h"

void BaseZone::componentUploaded(BaseExecutor* const executor, unsigned int numComponents)
{
	//uses memory_order_acq_rel because it needs to see all loaded parts if it's the last load job and release it's load job otherwise
	auto oldNumComponentsLoaded = this->numComponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel);
	if (oldNumComponentsLoaded == numComponents - 1u)
	{
		this->numComponentsLoaded.store(0u, std::memory_order::memory_order_relaxed);
		finishedCreatingNewState(executor);
	}
}

void BaseZone::setState(unsigned int newState, BaseExecutor* executor, SharedResources& sharedResources)
{
	unsigned int actualNewState = newState > this->highestSupportedState ? this->highestSupportedState : newState;
	if (this->nextState == undefinedState)
	{
		//we aren't currently changing state
		if (this->currentState != actualNewState)
		{
			this->nextState = actualNewState;
			this->newState = actualNewState;
			this->vTable->createNewStateData(this, executor, sharedResources);
		}
	}
	else
	{
		this->nextState = actualNewState;
	}
}

void BaseZone::finishedCreatingNewState(BaseExecutor* executor)
{
	executor->renderJobQueue().push(Job(this, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
	{
		auto zone = (BaseZone*)(requester);
		if (zone->currentState >= zone->highestSupportedState)
		{
			zone->start(executor, sharedResources);
		}
		unsigned int oldState = zone->currentState;
		void* oldData = zone->currentData;
		zone->currentState = zone->newState;
		zone->currentData = zone->newData;
		zone->newData = oldData;
		zone->newState = oldState;

		executor->gpuCompletionEventManager.addRequest(zone, zone->vTable->deleteOldStateData, sharedResources.graphicsEngine.frameIndex);//wait for cpu and gpu to stop using old state
	}));
}

void BaseZone::finishedDeletingOldState(BaseExecutor* executor)
{
	executor->renderJobQueue().push(Job(this, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)//wait for zone to start using new state
	{
		auto zone = (BaseZone*)(requester);
		unsigned int newState = zone->currentState;
		if (zone->nextState != newState)
		{
			zone->nextState = newState;
			zone->newState = zone->nextState;
			zone->vTable->createNewStateData(zone, executor, sharedResources);
		}
		else
		{
			zone->nextState = undefinedState;
		}
	}));
}

void BaseZone::start(BaseExecutor* const executor, SharedResources& sharedResources)
{
	vTable->start(this, executor, sharedResources);
}

void BaseZone::loadConnectedAreas(BaseExecutor* executor, SharedResources& sharedResources, float distanceSquared, Area::VisitedNode* loadedAreas)
{
	vTable->loadConnectedAreas(this, executor, sharedResources, distanceSquared, loadedAreas);
}

bool BaseZone::changeArea(BaseExecutor* executor, SharedResources& sharedResources)
{
	return vTable->changeArea(this, executor, sharedResources);
}