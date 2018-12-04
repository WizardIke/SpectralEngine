#pragma once

#include <atomic>
#include <limits>
#include "Area.h"
#undef min
#undef max

template<class ThreadResources, class GlobalResources>
class Zone
{
	struct VTable
	{
		void(*start)(Zone& zone, ThreadResources& threadResources, GlobalResources& globalResources);
		void (*createNewStateData)(Zone& zone, ThreadResources& threadResources, GlobalResources& globalResources);
		void(*deleteOldStateData)(void* zone, void* threadResources, void* GlobalResources);
		void(*loadConnectedAreas)(Zone& zone, ThreadResources& threadResources, GlobalResources& globalResources, float distanceFromAreaEntrance, Area::VisitedNode* loadedAreas);
		bool(*changeArea)(Zone& zone, ThreadResources& threadResources, GlobalResources& globalResources);
	};
	const VTable* const vTable;

	constexpr static unsigned int undefinedState = std::numeric_limits<unsigned int>::max();
	
	Zone(const VTable* const vTable, unsigned int highestSupportedState) : vTable(vTable), highestSupportedState(highestSupportedState), currentState(highestSupportedState), nextState(undefinedState) {}
public:
	Zone(std::nullptr_t) : vTable(nullptr) {}
	Zone(Zone&& other) : vTable(other.vTable), highestSupportedState(other.highestSupportedState), currentState(other.currentState), nextState(other.nextState) {} // should be deleted when c++17 becomes common. Only used for return from factory methods

	template<class ZoneFunctions>
	static Zone create(unsigned int numberOfStates)
	{
		static const VTable table
		{
			ZoneFunctions::start,
			ZoneFunctions::createNewStateData,
			[](void* zone, void* threadResources, void* globalResources) {ZoneFunctions::deleteOldStateData(*(Zone*)zone, *(ThreadResources*)threadResources, *(GlobalResources*)globalResources); },
			ZoneFunctions::loadConnectedAreas,
			ZoneFunctions::changeArea
		};
		return Zone(&table, numberOfStates);
	}

	~Zone() {}

	operator bool() {return vTable != nullptr;}
	
	void* currentData; //data that is currently getting used
	void* lastUsedData;
	union //data waiting to be used or deleted
	{
		void* newData; 
		void* oldData;
	};
	
	unsigned int currentState; //the state that is currenly getting used
	unsigned int lastUsedState;
	unsigned int nextState; //the state that we want to be in
	union //state waiting to be used or deleted
	{
		unsigned int newState;
		unsigned int oldState;
	};
	unsigned int highestSupportedState;
	std::atomic<unsigned int> numComponentsLoaded = 0;

	void componentUploaded(ThreadResources& threadResources, unsigned int numComponents)
	{
		//uses memory_order_acq_rel because it needs to see all loaded parts if it's the last load job and release it's load job otherwise
		auto oldNumComponentsLoaded = this->numComponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel);
		if (oldNumComponentsLoaded == numComponents - 1u)
		{
			this->numComponentsLoaded.store(0u, std::memory_order::memory_order_relaxed);
			finishedCreatingNewState(threadResources);
		}
	}

	void finishMovingToUnloadedState(ThreadResources& threadResources)
	{
		threadResources.taskShedular.update2NextQueue().push({ this, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			Zone& zone = *(Zone*)(context);
			unsigned int oldState = zone.currentState;
			void* oldData = zone.currentData;
			zone.currentState = zone.newState;
			zone.oldData = oldData;
			zone.oldState = oldState;

			threadResources.gpuCompletionEventManager.addRequest(&zone, zone.vTable->deleteOldStateData, globalResources.graphicsEngine.frameIndex);//wait for cpu and gpu to stop using old state
		} });
	}

	void setState(unsigned int newState1, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		const unsigned int highestSupported = this->highestSupportedState;
		unsigned int actualNewState = newState1 > highestSupported ? highestSupported : newState1;
		if (this->nextState == undefinedState)
		{
			//we aren't currently changing state
			if (this->currentState != actualNewState)
			{
				this->nextState = actualNewState;
				this->newState = actualNewState;
				if (actualNewState == highestSupported)
				{
					finishMovingToUnloadedState(threadResources);
				}
				else
				{
					this->vTable->createNewStateData(*this, threadResources, globalResources);
				}
			}
		}
		else
		{
			this->nextState = actualNewState;
		}
	}

	void finishedCreatingNewState(ThreadResources& threadResources)
	{
		//Not garantied to be next queue. Could be current queue.
		threadResources.taskShedular.update2NextQueue().concurrentPush({ this, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			Zone& zone = *(Zone*)(context);
			unsigned int oldState = zone.currentState;
			void* oldData = zone.currentData;
			zone.currentState = zone.newState;
			zone.currentData = zone.newData;
			zone.oldData = oldData;
			zone.oldState = oldState;

			if (oldState >= zone.highestSupportedState)
			{
				unsigned int currentState = zone.currentState;
				if (zone.nextState != currentState)
				{
					zone.newState = zone.nextState;
					if (zone.nextState == zone.highestSupportedState)
					{
						zone.currentState = zone.highestSupportedState;
						zone.nextState = undefinedState;
					}
					else
					{
						zone.start(threadResources, globalResources);
						zone.vTable->createNewStateData(zone, threadResources, globalResources);
					}
				}
				else
				{
					zone.start(threadResources, globalResources);
					zone.nextState = undefinedState;
				}
			}
			else
			{
				threadResources.gpuCompletionEventManager.addRequest(&zone, zone.vTable->deleteOldStateData, globalResources.graphicsEngine.frameIndex);//wait for cpu and gpu to stop using old state
			}
		} });
	}

	void finishedDeletingOldState(ThreadResources& threadResources)
	{
		numComponentsLoaded.store(0u, std::memory_order::memory_order_relaxed);
		//Not garantied to be next queue. Could be current queue.
		//wait for zone to start using new state
		threadResources.taskShedular.update2NextQueue().concurrentPush({ this, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			Zone& zone = *(Zone*)(context);
			unsigned int currentState = zone.currentState;
			if (zone.nextState != currentState)
			{
				zone.newState = zone.nextState;
				if (zone.nextState == zone.highestSupportedState)
				{
					zone.finishMovingToUnloadedState(threadResources);
				}
				else
				{
					zone.vTable->createNewStateData(zone, threadResources, globalResources);
				}
			}
			else
			{
				zone.nextState = undefinedState;
			}
		} });
	}

	void start(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		vTable->start(*this, threadResources, globalResources);
	}

	void loadConnectedAreas(ThreadResources& threadResources, GlobalResources& globalResources, float distanceFromAreaEntrance, Area::VisitedNode* loadedAreas)
	{
		vTable->loadConnectedAreas(*this, threadResources, globalResources, distanceFromAreaEntrance, loadedAreas);
	}

	bool changeArea(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		return vTable->changeArea(*this, threadResources, globalResources);
	}
};