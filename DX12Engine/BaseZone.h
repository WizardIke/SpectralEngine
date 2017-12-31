#pragma once

#include <atomic>
#include <mutex>
#include "Spinlock.h"
#include <d3d12.h>
#include "Mesh.h"
#include <limits>
#include "Area.h"
class BaseExecutor;
#undef max

class BaseZone
{
	class VTable
	{
	public:
		void (*loadHighDetail)(BaseZone* zone, BaseExecutor* const executor);
		void (*loadMediumDetail)(BaseZone* zone, BaseExecutor* const executor);
		void (*loadLowDetail)(BaseZone* zone, BaseExecutor* const executor);

		void (*unloadHighDetail)(void* zone, BaseExecutor* const executor);
		void (*unloadMediumDetail)(void* zone, BaseExecutor* const executor);
		void (*unloadLowDetail)(void* zone, BaseExecutor* const executor);


		void(*loadConnectedAreas)(BaseZone* zone, BaseExecutor* const executor, float distanceSquared, Area::VisitedNode* loadedAreas);
		bool(*changeArea)(BaseZone* zone, BaseExecutor* const executor);

		void (*start)(BaseZone* zone, BaseExecutor* const executor);
	};
	const VTable* const vTable;

	void unloadHighDetail(BaseExecutor* executor);
	void unloadMediumDetail(BaseExecutor* executor);
	void unloadLowDetail(BaseExecutor* executor);
public:
	BaseZone(const VTable* const vTable) : vTable(vTable) {}
	BaseZone(BaseZone&& other) : vTable(other.vTable) {} // should be deleted when c++17 becomes common. Only used for factory methods

	template<class Zone>
	static BaseZone create()
	{
		static const VTable table
		{	
			Zone::loadHighDetailJobs, Zone::loadMediumDetailJobs, Zone::loadLowDetailJobs,

			[](void* zone, BaseExecutor* const executor) {Zone::unloadHighDetailJobs(reinterpret_cast<BaseZone*>(zone), executor); },
			[](void* zone, BaseExecutor* const executor) {Zone::unloadMediumDetailJobs(reinterpret_cast<BaseZone*>(zone), executor); },
			[](void* zone, BaseExecutor* const executor) {Zone::unloadLowDetailJobs(reinterpret_cast<BaseZone*>(zone), executor); },

			Zone::loadConnectedAreas, Zone::changeArea, 
			Zone::start 
		};
		return BaseZone(&table);
	}
	~BaseZone();

	enum Lod : unsigned char
	{
		high,
		medium,
		low,
		unloaded,
		transitionHighToMedium,
		transitionHighToLow,
		transitionHighToUnloaded,
		transitionMediumToHigh,
		transitionMediumToLow,
		transitionMediumToUnloaded,
		transitionLowToHigh,
		transitionLowToMedium,
		transitionLowToUnloaded,
	};

	std::mutex loadingMutex;
	void* currentResources = nullptr;
	void* nextResources = nullptr;
	std::atomic<Lod> levelOfDetail = unloaded;
	Lod nextLevelOfDetail = unloaded;
	bool transitioning = false;


	template<Lod nextLod, Lod lowestLod>
	void lastComponentLoaded(BaseExecutor* const executor)
	{
		if (nextLod == high)
		{
			loadingMutex.lock();
			Lod oldLevelOfDetail = levelOfDetail.load(std::memory_order::memory_order_acquire);
			if (oldLevelOfDetail == nextLevelOfDetail)
			{
				loadingMutex.unlock();
				unloadHighDetail(executor);
			}
			else if (oldLevelOfDetail == medium)
			{
				if (lowestLod < medium) // this zone only supports high lod
				{
					levelOfDetail.store(high, std::memory_order::memory_order_release);
					std::swap(currentResources, nextResources);
					transitioning = false;
					loadingMutex.unlock();
					start(executor);
				}
				else
				{
					levelOfDetail.store(transitionMediumToHigh, std::memory_order::memory_order_release);
					loadingMutex.unlock();
				}
			}
			else if (oldLevelOfDetail == low)
			{
				if (lowestLod < low) // this zone only supports high and medium lod
				{
					levelOfDetail.store(high, std::memory_order::memory_order_release);
					std::swap(currentResources, nextResources);
					transitioning = false;
					loadingMutex.unlock();
					start(executor);
				}
				else
				{
					levelOfDetail.store(transitionLowToHigh, std::memory_order::memory_order_release);
					loadingMutex.unlock();
				}
			}
			else
			{
				levelOfDetail.store(high, std::memory_order::memory_order_release);
				if (nextLevelOfDetail == high)
				{
					transitioning = false;
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					start(executor);
				}
				else if (nextLevelOfDetail == medium)
				{
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					start(executor);
					vTable->loadMediumDetail(this, executor);
				}
				else //else if (nextLevelOfDetail == low) //nextLevelOfDetail == none has already been handled
				{
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					start(executor);
					vTable->loadLowDetail(this, executor);
				}
			}
		}
		else if (nextLod == medium)
		{
			loadingMutex.lock();
			Lod oldLevelOfDetail = levelOfDetail.load(std::memory_order::memory_order_acquire);
			if (oldLevelOfDetail == nextLevelOfDetail)
			{
				loadingMutex.unlock();
				unloadMediumDetail(executor);
			}
			else if (oldLevelOfDetail == high)
			{
				levelOfDetail.store(transitionHighToMedium, std::memory_order::memory_order_release);
				loadingMutex.unlock();
			}
			else if (oldLevelOfDetail == low)
			{
				if (lowestLod < low) // this zone only supports high and medium lod
				{
					levelOfDetail.store(medium, std::memory_order::memory_order_release);
					std::swap(currentResources, nextResources);
					transitioning = false;
					loadingMutex.unlock();
					start(executor);
				}
				else
				{
					levelOfDetail.store(transitionLowToMedium, std::memory_order::memory_order_release);
					loadingMutex.unlock();
				}
			}
			else
			{
				levelOfDetail.store(medium, std::memory_order::memory_order_release);

				if (nextLevelOfDetail == medium)
				{
					transitioning = false;
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					if (lowestLod >= medium) // this zone supports more than just high lod
					{
						start(executor);
					}
				}
				else if (nextLevelOfDetail == high)
				{
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					if (lowestLod >= medium) // this zone supports more than just high lod
					{
						start(executor);
					}
					vTable->loadHighDetail(this, executor);
				}
				else //else if (nextLevelOfDetail == low) //nextLevelOfDetail == none has already been handled
				{
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					if (lowestLod >= medium) // this zone supports more than just high lod
					{
						start(executor);
					}
					vTable->loadLowDetail(this, executor);
				}
			}
		}
		else
		{
			loadingMutex.lock();
			Lod oldLevelOfDetail = levelOfDetail.load(std::memory_order::memory_order_acquire);
			if (oldLevelOfDetail == nextLevelOfDetail)
			{
				loadingMutex.unlock();
				unloadLowDetail(executor);
			}
			else if (oldLevelOfDetail == high)
			{
				levelOfDetail.store(transitionHighToLow, std::memory_order::memory_order_release);
				loadingMutex.unlock();
			}
			else if (oldLevelOfDetail == medium)
			{
				if (lowestLod < medium) // this zone only supports high lod 
				{
					levelOfDetail.store(low, std::memory_order::memory_order_release);
					transitioning = false;
					loadingMutex.unlock();
				}
				else
				{
					levelOfDetail.store(transitionMediumToLow, std::memory_order::memory_order_release);
					loadingMutex.unlock();
				}
			}
			else
			{
				levelOfDetail.store(low, std::memory_order::memory_order_release); 
				if (nextLevelOfDetail == low)
				{
					transitioning = false;
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					if (lowestLod >= low)
					{
						start(executor);
					}
				}
				else if (nextLevelOfDetail == high)
				{
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					if (lowestLod >= low)
					{
						start(executor);
					}
					vTable->loadHighDetail(this, executor);
				}
				else //else if (nextLevelOfDetail == medium) //nextLevelOfDetail == unloaded has already been handled
				{
					std::swap(currentResources, nextResources);
					loadingMutex.unlock();
					if (lowestLod >= low)
					{
						start(executor);
					}
					vTable->loadMediumDetail(this, executor);
				}
			}
		}
	}

	template<Lod previousLod, Lod nextLod>
	void transition(BaseExecutor* executor)
	{
		{
			std::lock_guard<decltype(loadingMutex)> lock(loadingMutex);
			std::swap(currentResources, nextResources);
			levelOfDetail.store(nextLod, std::memory_order::memory_order_release);
		}

		if (previousLod == high)
		{
			unloadHighDetail(executor);
		}
		else if (previousLod == medium)
		{
			unloadMediumDetail(executor);
		}
		else
		{
			unloadLowDetail(executor);
		}
	}

	template<Lod lowestLod>
	void lastComponentUnloaded(BaseExecutor* const executor)
	{
		loadingMutex.lock();
		Lod currentLevelOfDetail = levelOfDetail.load(std::memory_order::memory_order_acquire);
		if (currentLevelOfDetail == nextLevelOfDetail)
		{
			transitioning = false;
			loadingMutex.unlock();
		}
		else if (currentLevelOfDetail == high)
		{
			if (nextLevelOfDetail == medium)
			{
				loadingMutex.unlock();
				vTable->loadMediumDetail(this, executor);
			}
			else if (nextLevelOfDetail == low)
			{
				loadingMutex.unlock();
				vTable->loadLowDetail(this, executor);
			}
			else
			{
				levelOfDetail.store(transitionHighToUnloaded, std::memory_order::memory_order_release);
				loadingMutex.unlock();
			}
		}
		else if (currentLevelOfDetail == medium)
		{
			if (nextLevelOfDetail == high)
			{
				loadingMutex.unlock();
				vTable->loadHighDetail(this, executor);
			}
			else if (nextLevelOfDetail == low)
			{
				loadingMutex.unlock();
				vTable->loadLowDetail(this, executor);
			}
			else
			{
				if (lowestLod < medium)
				{
					loadingMutex.unlock();
					unloadMediumDetail(executor);
				}
				else
				{
					levelOfDetail.store(transitionMediumToUnloaded, std::memory_order::memory_order_release);
					loadingMutex.unlock();
				}
			}
		}
		else if(currentLevelOfDetail == low)
		{
			if (nextLevelOfDetail == high)
			{
				loadingMutex.unlock();
				vTable->loadHighDetail(this, executor);
			}
			else if (nextLevelOfDetail == medium)
			{
				loadingMutex.unlock();
				vTable->loadMediumDetail(this, executor);
			}
			else
			{
				if (lowestLod < low)
				{
					loadingMutex.unlock();
					unloadMediumDetail(executor);
				}
				else
				{
					levelOfDetail.store(transitionLowToUnloaded, std::memory_order::memory_order_release);
					loadingMutex.unlock();
				}
			}
		}
		else
		{
			if (nextLevelOfDetail == high)
			{
				loadingMutex.unlock();
				vTable->loadHighDetail(this, executor);
			}
			else if (nextLevelOfDetail == medium)
			{
				loadingMutex.unlock();
				vTable->loadMediumDetail(this, executor);
			}
			else
			{
				loadingMutex.unlock();
				vTable->loadLowDetail(this, executor);
			}
		}
	}

	template<Lod nextLod, Lod lowestLod>
	static void componentUploaded(BaseZone* zone, BaseExecutor* const executor, std::atomic<unsigned char>& numComponentsLoaded, unsigned char numComponents)
	{
		//uses memory_order_acq_rel because it needs to see all loaded parts if it's the last load job and release it's load job otherwise
		auto oldNumComponentsLoaded = numComponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel); 
		if (oldNumComponentsLoaded == numComponents - 1u)
		{
			zone->lastComponentLoaded<nextLod, lowestLod>(executor);
		}
	}

	operator bool() {return vTable != nullptr;}

	void loadHighDetail(BaseExecutor* const executor);
	void loadMediumDetail(BaseExecutor* const executor);
	void loadLowDetail(BaseExecutor* const executor);
	void unloadAll(BaseExecutor* executor);

	void loadConnectedAreas(BaseExecutor* const executor, float distanceSquared, Area::VisitedNode* loadedAreas) 
	{
		vTable->loadConnectedAreas(this, executor, distanceSquared, loadedAreas);
	}
	bool changeArea(BaseExecutor* const executor)
	{
		return vTable->changeArea(this, executor);
	}

	void start(BaseExecutor* const executor)
	{
		vTable->start(this, executor);
	}
};