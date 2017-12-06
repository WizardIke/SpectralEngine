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
		void (*loadHighDetailJobs)(BaseZone* zone, BaseExecutor* const executor);
		void (*loadMediumDetailJobs)(BaseZone* zone, BaseExecutor* const executor);
		void (*loadLowDetailJobs)(BaseZone* zone, BaseExecutor* const executor);

		void (*unloadHighDetailJobs)(BaseZone* zone, BaseExecutor* const executor);
		void (*unloadMediumDetailJobs)(BaseZone* zone, BaseExecutor* const executor);
		void (*unloadLowDetailJobs)(BaseZone* zone, BaseExecutor* const executor);


		void(*loadConnectedAreas)(BaseZone* zone, BaseExecutor* const executor, float distanceSquared, Area::VisitedNode* loadedAreas);
		bool(*changeArea)(BaseZone* zone, BaseExecutor* const executor);

		void (*start)(BaseZone* zone, BaseExecutor* const executor);
	};
	const VTable* const vTable;
public:
	BaseZone(const VTable* const vTable) : vTable(vTable) {}
	BaseZone(BaseZone&& other) : vTable(other.vTable) {}

	template<class Zone>
	static BaseZone create()
	{
		static const VTable table
		{	
			&Zone::loadHighDetailJobs , &Zone::loadMediumDetailJobs , &Zone::loadLowDetailJobs ,
			&Zone::unloadHighDetailJobs , &Zone::unloadMediumDetailJobs , &Zone::unloadLowDetailJobs ,
			&Zone::loadConnectedAreas, &Zone::changeArea, 
			&Zone::start 
		};
		return BaseZone(&table);
	}
	~BaseZone();

	enum Lod
	{
		high = 0u,
		medium = 1u,
		low = 2u,
		unloaded = std::numeric_limits<unsigned char>::max(),
	};

	template<Lod lod>
	void lastComponentLoaded(BaseExecutor* const executor)
	{
		if (lod == high)
		{
			loadingMutex.lock();
			if (levelOfDetail == nextLevelOfDetail)
			{
				loadingMutex.unlock();
				unloadHighDetail(executor);
			}
			else if (levelOfDetail == medium)
			{
				levelOfDetail = high;
				loadingMutex.unlock();
				unloadMediumDetail(executor);
			}
			else if (levelOfDetail == low)
			{
				levelOfDetail = high;
				loadingMutex.unlock();
				unloadLowDetail(executor);
			}
			else
			{
				levelOfDetail = high;
				if (nextLevelOfDetail == high)
				{
					transitioning = false;
					loadingMutex.unlock();
					start(executor);
				}
				else if (nextLevelOfDetail == medium)
				{
					loadingMutex.unlock();
					loadMediumDetail(executor);
				}
				else //else if (nextLevelOfDetail == low) //nextLevelOfDetail == none has already been handled
				{
					loadingMutex.unlock();
					loadLowDetail(executor);
				}
			}
		}
		else if (lod == medium)
		{
			loadingMutex.lock();
			if (levelOfDetail == nextLevelOfDetail)
			{
				loadingMutex.unlock();
				unloadMediumDetail(executor);
			}
			else if (levelOfDetail == high)
			{
				levelOfDetail = medium;
				loadingMutex.unlock();
				unloadHighDetail(executor);
			}
			else if (levelOfDetail == low)
			{
				levelOfDetail = medium;
				loadingMutex.unlock();
				unloadLowDetail(executor);
			}
			else
			{
				levelOfDetail = medium;
				if (nextLevelOfDetail == medium)
				{
					transitioning = false;
					loadingMutex.unlock();
					start(executor);
				}
				else if (nextLevelOfDetail == high)
				{
					loadingMutex.unlock();
					loadHighDetail(executor);
				}
				else //else if (nextLevelOfDetail == low) //nextLevelOfDetail == none has already been handled
				{
					loadingMutex.unlock();
					loadLowDetail(executor);
				}
			}
		}
		else
		{
			loadingMutex.lock();
			if (levelOfDetail == nextLevelOfDetail)
			{
				loadingMutex.unlock();
				unloadLowDetail(executor);
			}
			else if (levelOfDetail == high)
			{
				levelOfDetail = low;
				loadingMutex.unlock();
				unloadHighDetail(executor);
			}
			else if (levelOfDetail == medium)
			{
				levelOfDetail = low;
				loadingMutex.unlock();
				unloadMediumDetail(executor);
			}
			else
			{
				levelOfDetail = low;
				if (nextLevelOfDetail == low)
				{
					transitioning = false;
					loadingMutex.unlock();
					start(executor);
				}
				else if (nextLevelOfDetail == high)
				{
					loadingMutex.unlock();
					loadHighDetail(executor);
				}
				else //else if (nextLevelOfDetail == medium) //nextLevelOfDetail == unloaded has already been handled
				{
					loadingMutex.unlock();
					loadMediumDetail(executor);
				}
			}
		}
	}

	template<Lod lod>
	void lastComponentUnloaded(BaseExecutor* const executor)
	{
		if (lod == high)
		{
			loadingMutex.lock();
			if (levelOfDetail == nextLevelOfDetail)
			{
				transitioning = false;
				loadingMutex.unlock();
			}
			else if (nextLevelOfDetail == medium)
			{
				loadingMutex.unlock();
				loadMediumDetail(executor);
			}
			else if (nextLevelOfDetail == low)
			{
				loadingMutex.unlock();
				loadLowDetail(executor);
			}
			else
			{
				levelOfDetail = unloaded;
				loadingMutex.unlock();
				unloadHighDetail(executor);
			}
		}
		else if (lod == medium)
		{
			loadingMutex.lock();
			if (levelOfDetail == nextLevelOfDetail)
			{
				transitioning = false;
				loadingMutex.unlock();
			}
			else if (nextLevelOfDetail == high)
			{
				loadingMutex.unlock();
				loadHighDetail(executor);
			}
			else if (nextLevelOfDetail == low)
			{
				loadingMutex.unlock();
				loadLowDetail(executor);
			}
			else
			{
				levelOfDetail = unloaded;
				loadingMutex.unlock();
				unloadMediumDetail(executor);
			}
		}
		else
		{
			loadingMutex.lock();
			if (levelOfDetail == nextLevelOfDetail)
			{
				transitioning = false;
				loadingMutex.unlock();
			}
			else if (nextLevelOfDetail == high)
			{
				loadingMutex.unlock();
				loadHighDetail(executor);
			}
			else if (nextLevelOfDetail == medium)
			{
				loadingMutex.unlock();
				loadMediumDetail(executor);
			}
			else
			{
				levelOfDetail = unloaded;
				loadingMutex.unlock();
				unloadLowDetail(executor);
			}
		}
	}

	template<Lod lod, unsigned char numComponents>
	static void componentUploaded(BaseZone* zone, BaseExecutor* const executor, std::atomic<unsigned char>& numComponentsLoaded)
	{
		auto oldNumComponentsLoaded = numComponentsLoaded.fetch_add(1u, std::memory_order_relaxed);
		if (oldNumComponentsLoaded == numComponents - 1u)
		{
			zone->lastComponentLoaded<lod>(executor);
		}
	}

	std::mutex loadingMutex;
	void* currentResources = nullptr;
	void* nextResources = nullptr;
	std::atomic<unsigned char> levelOfDetail = unloaded;
	unsigned char nextLevelOfDetail = unloaded;
	bool transitioning = false;


	operator bool() {return vTable != nullptr;}

	void loadHighDetail(BaseExecutor* const executor);
	void loadMediumDetail(BaseExecutor* const executor);
	void loadLowDetail(BaseExecutor* const executor);

	void unloadHighDetail(BaseExecutor* executor);
	void unloadMediumDetail(BaseExecutor* executor);
	void unloadLowDetail(BaseExecutor* executor);
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