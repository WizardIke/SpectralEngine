#pragma once

#include <atomic>
#include <mutex>
#include "Spinlock.h"
#include <d3d12.h>
#include "Mesh.h"
#include <limits>
#include "Area.h"
class BaseExecutor;
class SharedResources;
#undef min
#undef max

class BaseZone
{
	struct VTable
	{
		void(*start)(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources);
		void (*createNewStateData)(BaseZone* zone, BaseExecutor* executor, SharedResources& sharedResources);
		void(*deleteOldStateData)(void* zone, BaseExecutor* executor, SharedResources& sharedResources);
		void(*loadConnectedAreas)(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources, float distanceSquared, Area::VisitedNode* loadedAreas);
		bool(*changeArea)(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources);
	};
	const VTable* const vTable;

	constexpr static unsigned int undefinedState = std::numeric_limits<unsigned int>::max();
	
	BaseZone(const VTable* const vTable, unsigned int highestSupportedState) : vTable(vTable), highestSupportedState(highestSupportedState) {}
public:
	BaseZone(std::nullptr_t) : vTable(nullptr) {}
	BaseZone(BaseZone&& other) : vTable(other.vTable) {} // should be deleted when c++17 becomes common. Only used for return from factory methods

	template<class Zone>
	static BaseZone create(unsigned int numberOfStates)
	{
		static const VTable table
		{
			Zone::start,
			Zone::createNewStateData,
			[](void* zone, BaseExecutor* executor, SharedResources& sharedResources) {Zone::deleteOldStateData((BaseZone*)zone, executor, sharedResources); },
			Zone::loadConnectedAreas,
			Zone::changeArea 
		};
		return BaseZone(&table, numberOfStates);
	}

	~BaseZone() {}

	operator bool() {return vTable != nullptr;}
	
	void* currentData; //data that is currently getting used
	void* lastUsedData;
	union //data waiting to be used or deleted
	{
		void* newData; 
		void* oldData;
	};
	
	unsigned int currentState = undefinedState; //the state that is currenly getting used
	unsigned int lastUsedState;
	unsigned int nextState = undefinedState; //the state that we want to be in
	union //state waiting to be used or deleted
	{
		unsigned int newState;
		unsigned int oldState;
	};
	unsigned int highestSupportedState;
	std::atomic<unsigned int> numComponentsLoaded = 0;

	void componentUploaded(BaseExecutor* const executor, unsigned int numComponents);
	void setState(unsigned int newState, BaseExecutor* executor, SharedResources& sharedResources);
	void finishedCreatingNewState(BaseExecutor* executor);
	void finishedDeletingOldState(BaseExecutor* executor);
	void start(BaseExecutor* const executor, SharedResources& sharedResources);
	void loadConnectedAreas(BaseExecutor* executor, SharedResources& sharedResources, float distanceSquared, Area::VisitedNode* loadedAreas);
	bool changeArea(BaseExecutor* executor, SharedResources& sharedResources);
};