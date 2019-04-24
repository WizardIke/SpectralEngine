#pragma once

#include <atomic>
#include <limits>
#include "Range.h"
#include "Portal.h"
#include "WorldManagerStopRequest.h"
#include "LinkedTask.h"
#undef min
#undef max

template<class ThreadResources, class GlobalResources>
class Zone : private LinkedTask
{
	struct VTable
	{
		void(*start)(Zone& zone, ThreadResources& threadResources, GlobalResources& globalResources);
		void (*createNewStateData)(Zone& zone, ThreadResources& threadResources, GlobalResources& globalResources);
		void(*deleteOldStateData)(Zone& zone, ThreadResources& threadResources, GlobalResources& globalResources);
		Range<Portal*>(*getPortals)(Zone& zone);
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
			ZoneFunctions::deleteOldStateData,
			ZoneFunctions::getPortals
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
private:
	std::atomic<unsigned int> numComponentsLoaded = 0;
	WorldManagerStopRequest* stopRequest = nullptr;
public:

	void componentUploaded(ThreadResources& threadResources, GlobalResources& globalResources, unsigned int numComponents)
	{
		//uses memory_order_acq_rel because it needs to see all loaded parts if it's the last load job and release it's load job otherwise
		auto oldNumComponentsLoaded = this->numComponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel);
		if (oldNumComponentsLoaded == numComponents - 1u)
		{
			this->numComponentsLoaded.store(0u, std::memory_order::memory_order_relaxed);
			finishedCreatingNewState(threadResources, globalResources);
		}
	}
private:
	void moveToUnloadedState(ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushPrimaryTask(1u, { this, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			Zone& zone = *(Zone*)(context);
			unsigned int oldState = zone.currentState;
			void* oldData = zone.currentData;
			zone.currentState = zone.newState;
			zone.oldData = oldData;
			zone.oldState = oldState;

			zone.vTable->deleteOldStateData(zone, threadResources, globalResources);
		} });
	}
public:
	//Must be called dueing update1
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
					moveToUnloadedState(threadResources);
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

	//Must be called dueing update1
	void stop(WorldManagerStopRequest* stopRequest1, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		unsigned int actualNewState = highestSupportedState;
		stopRequest = stopRequest1;
		if(this->nextState == undefinedState)
		{
			//we aren't currently changing state
			if(this->currentState != actualNewState)
			{
				this->nextState = actualNewState;
				this->newState = actualNewState;
				moveToUnloadedState(threadResources);
			}
			else
			{
				stopRequest1->callback(*stopRequest1, &threadResources, &globalResources);
			}
		}
		else
		{
			this->nextState = actualNewState;
		}
	}

	void finishedCreatingNewState(ThreadResources&, GlobalResources& globalResources)
	{
		execute = [](LinkedTask& task, void* tr, void* gr)
		{
			Zone& zone = static_cast<Zone&>(task);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			unsigned int oldState = zone.currentState;
			void* oldData = zone.currentData;
			zone.currentState = zone.newState;
			zone.currentData = zone.newData;
			zone.oldData = oldData;
			zone.oldState = oldState;

			if(oldState >= zone.highestSupportedState)
			{
				unsigned int currentState = zone.currentState;
				if(zone.nextState != currentState)
				{
					zone.newState = zone.nextState;
					if(zone.nextState == zone.highestSupportedState)
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
				zone.vTable->deleteOldStateData(zone, threadResources, globalResources);
			}
		};
		globalResources.taskShedular.pushPrimaryTaskFromOtherThread(1u, *this);
	}

	void finishedDeletingOldState(ThreadResources&, GlobalResources& globalResources)
	{
		//Not garantied to be next queue. Could be current queue.
		//wait for zone to start using new state
		execute = [](LinkedTask& task, void* tr, void* gr)
		{
			Zone& zone = static_cast<Zone&>(task);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			unsigned int currentState = zone.currentState;
			if(zone.nextState != currentState)
			{
				zone.newState = zone.nextState;
				if(zone.nextState == zone.highestSupportedState)
				{
					zone.moveToUnloadedState(threadResources);
				}
				else
				{
					zone.vTable->createNewStateData(zone, threadResources, globalResources);
				}
			}
			else
			{
				zone.nextState = undefinedState;
				auto stopRequest1 = zone.stopRequest;
				if(stopRequest1 != nullptr)
				{
					stopRequest1->callback(*stopRequest1, &threadResources, &globalResources);
				}
			}
		};
		globalResources.taskShedular.pushPrimaryTaskFromOtherThread(1u, *this);
	}

	void start(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		vTable->start(*this, threadResources, globalResources);
	}

	Range<Portal*> getPortals()
	{
		return vTable->getPortals(*this);
	}

	void executeWhenGpuFinishesCurrentFrame(GlobalResources& globalResources, void(*task)(LinkedTask& task, void* tr, void* gr))
	{
		execute = task;
		globalResources.graphicsEngine.executeWhenGpuFinishesCurrentFrame(*this); //wait for cpu and gpu to stop using old state
	}

	static Zone& from(LinkedTask& task)
	{
		return static_cast<Zone&>(task);
	}
};