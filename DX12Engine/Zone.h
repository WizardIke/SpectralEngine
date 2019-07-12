#pragma once

#include <atomic>
#include <limits>
#include "Range.h"
#include "Portal.h"
#include "WorldManagerStopRequest.h"
#include "LinkedTask.h"
#include "TaskShedular.h"
#include "GraphicsEngine.h"
#undef min
#undef max

template<class ThreadResources>
class Zone : private LinkedTask
{
	struct VTable
	{
		void(*start)(Zone& zone, ThreadResources& threadResources);
		void (*createNewStateData)(Zone& zone, ThreadResources& threadResources);
		void(*deleteOldStateData)(Zone& zone, ThreadResources& threadResources);
		Range<Portal*>(*getPortals)(Zone& zone);
	};
	const VTable* const vTable;

	constexpr static unsigned int undefinedState = std::numeric_limits<unsigned int>::max();
	
	Zone(const VTable* const vTable, unsigned int highestSupportedState, void* context) : vTable(vTable), highestSupportedState(highestSupportedState), currentState(highestSupportedState), nextState(undefinedState), context(context) {}
public:
	Zone(std::nullptr_t) : vTable(nullptr) {}

#if defined(_MSC_VER)
	/*
	Initialization of array members doesn't seam to have copy elision in some cases when it should in c++17.
	*/
	Zone(Zone<ThreadResources>&& other);
#endif

	template<class ZoneFunctions>
	static Zone create(unsigned int numberOfStates, void* context)
	{
		static const VTable table
		{
			ZoneFunctions::start,
			ZoneFunctions::createNewStateData,
			ZoneFunctions::deleteOldStateData,
			ZoneFunctions::getPortals
		};
		return Zone(&table, numberOfStates, context);
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
	void* context;

	void componentUploaded(TaskShedular<ThreadResources>& taskShedular, unsigned int numComponents)
	{
		//uses memory_order_acq_rel because it needs to see all loaded parts if it's the last load job and release it's load job otherwise
		auto oldNumComponentsLoaded = this->numComponentsLoaded.fetch_add(1u, std::memory_order_acq_rel);
		if (oldNumComponentsLoaded == numComponents - 1u)
		{
			this->numComponentsLoaded.store(0u, std::memory_order_relaxed);
			finishedCreatingNewState(taskShedular);
		}
	}
private:
	void moveToUnloadedState(ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushPrimaryTask(1u, { this, [](void* context, ThreadResources& threadResources)
		{
			Zone& zone = *(Zone*)(context);
			unsigned int oldState = zone.currentState;
			void* oldData = zone.currentData;
			zone.currentState = zone.newState;
			zone.oldData = oldData;
			zone.oldState = oldState;

			zone.vTable->deleteOldStateData(zone, threadResources);
		} });
	}
public:
	//Must be called dueing update1
	void setState(unsigned int newState1, ThreadResources& threadResources)
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
					this->vTable->createNewStateData(*this, threadResources);
				}
			}
		}
		else
		{
			this->nextState = actualNewState;
		}
	}

	//Must be called dueing update1
	void stop(WorldManagerStopRequest* stopRequest1, ThreadResources& threadResources)
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
				stopRequest1->onZoneStopped(&threadResources);
			}
		}
		else
		{
			this->nextState = actualNewState;
		}
	}

	void finishedCreatingNewState(TaskShedular<ThreadResources>& taskShedular)
	{
		execute = [](LinkedTask& task, void* tr)
		{
			Zone& zone = static_cast<Zone&>(task);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
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
					void* currentData = zone.currentData;

					zone.newState = zone.nextState;
					if(zone.nextState == zone.highestSupportedState)
					{
						zone.currentState = zone.highestSupportedState;
#ifndef NDEBUG
						zone.currentData = nullptr;
#endif
						zone.oldState = currentState;
						zone.oldData = currentData;
						zone.vTable->deleteOldStateData(zone, threadResources);
					}
					else
					{
						zone.start(threadResources);
						zone.vTable->createNewStateData(zone, threadResources);
					}
				}
				else
				{
					zone.start(threadResources);
					zone.nextState = undefinedState;
				}
			}
			else
			{
				zone.vTable->deleteOldStateData(zone, threadResources);
			}
		};
		taskShedular.pushPrimaryTaskFromOtherThread(1u, *this);
	}

	void finishedDeletingOldState(TaskShedular<ThreadResources>& taskShedular)
	{
		//Not garantied to be next queue. Could be current queue.
		//wait for zone to start using new state
		execute = [](LinkedTask& task, void* tr)
		{
			Zone& zone = static_cast<Zone&>(task);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
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
					zone.vTable->createNewStateData(zone, threadResources);
				}
			}
			else
			{
				zone.nextState = undefinedState;
				auto stopRequest1 = zone.stopRequest;
				if(stopRequest1 != nullptr)
				{
					stopRequest1->onZoneStopped(&threadResources);
				}
			}
		};
		taskShedular.pushPrimaryTaskFromOtherThread(1u, *this);
	}

	void start(ThreadResources& threadResources)
	{
		vTable->start(*this, threadResources);
	}

	Range<Portal*> getPortals()
	{
		return vTable->getPortals(*this);
	}

	void executeWhenGpuFinishesCurrentFrame(GraphicsEngine& graphicsEngine, void(*task)(LinkedTask& task, void* tr))
	{
		execute = task;
		graphicsEngine.executeWhenGpuFinishesCurrentFrame(*this); //wait for cpu and gpu to stop using old state
	}

	static Zone& from(LinkedTask& task)
	{
		return static_cast<Zone&>(task);
	}

	bool usesLevelOfDetail(unsigned int levelOfDetail)
	{
		return levelOfDetail < highestSupportedState;
	}
};