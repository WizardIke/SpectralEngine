#include "CaveArea.h"
#include <BaseExecutor.h>
#include <SharedResources.h>
#include "CaveZone1.h"

namespace Cave
{

	CaveArea::CaveArea() : 
		zones
		{
			Zone1()
		}
	{}

	void CaveArea::load(BaseExecutor* const executor, Vector2 position, float distance, Area::VisitedNode* loadedAreas)
	{
		Area::VisitedNode thisArea;
		thisArea.thisArea = this;
		if (!loadedAreas)
		{
			thisArea.next = nullptr;
			loadedAreas = &thisArea;
		}
		else
		{
			thisArea.next = loadedAreas;
			do
			{
				if (loadedAreas->thisArea == this) return; //this area has already been loaded
				loadedAreas = loadedAreas->next;
			} while (loadedAreas);
		}

		constexpr int numZonesRadius = Area::constexprCeil(mediumDetailRadius / Area::zoneDiameter);
		unsigned int minX, maxX, minZ, maxZ;
		minX = currentZoneX > numZonesRadius ? currentZoneX - numZonesRadius : 0u;
		maxX = currentZoneX + numZonesRadius < zonesLengthX - 1u ? currentZoneX + numZonesRadius : zonesLengthX - 1u;
		minZ = currentZoneZ > numZonesRadius ? currentZoneZ - numZonesRadius : 0u;
		maxZ = currentZoneZ + numZonesRadius < zonesLengthZ - 1u ? currentZoneZ + numZonesRadius : zonesLengthZ - 1u;

		float highDetailRadiusSquared = highDetailRadius - distance;
		if (highDetailRadiusSquared >= 0.0f) return;
		highDetailRadiusSquared *= highDetailRadiusSquared;

		float mediumDetailRadiusSquared = mediumDetailRadius - distance;
		mediumDetailRadiusSquared *= mediumDetailRadiusSquared;

		for (auto z = minZ; z <= maxZ; ++z)
		{
			for (auto x = minX; x <= maxX; ++x)
			{
				float difX, difZ;
				auto& zone = zones[z * zonesLengthX + x];
				if (!zone) continue;
				Vector2 zonePosition{ x * Area::zoneDiameter, z * Area::zoneDiameter };
				difX = position.x - zonePosition.x;
				difZ = position.y - zonePosition.y;
				float distanceSquared = difX * difX + difZ * difZ;

				if (distanceSquared < highDetailRadiusSquared)
				{
					zone.loadHighDetail(executor);
					zone.loadConnectedAreas(executor, distanceSquared, &thisArea);
				}
				else if (distanceSquared < mediumDetailRadiusSquared)
				{
					zone.loadMediumDetail(executor);
					zone.loadConnectedAreas(executor, distanceSquared, &thisArea);
				}
				else
				{
					zone.unloadAll(executor);
				}
			}
		}
	}

	void CaveArea::update(BaseExecutor* const executor)
	{
		auto& position = executor->sharedResources->playerPosition.location.position;
		Vector2 zonePosition{ currentZoneX * Area::zoneDiameter + 0.5f * Area::zoneDiameter, currentZoneZ * Area::zoneDiameter + 0.5f * Area::zoneDiameter };
		if (position.x > zonePosition.x + 0.5f * Area::zoneDiameter)
		{
			++currentZoneX;
			load(executor, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (position.x < zonePosition.x - 0.5f * Area::zoneDiameter)
		{
			--currentZoneX;
			load(executor, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (position.z > zonePosition.y + 0.5f * Area::zoneDiameter)
		{
			++currentZoneZ;
			load(executor, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (position.z < zonePosition.y - 0.5f * Area::zoneDiameter)
		{
			--currentZoneZ;
			load(executor, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (oldPosX < zonePosition.x && position.x >= zonePosition.x ||
			oldPosX >= zonePosition.x && position.x < zonePosition.x ||
			oldPosZ < zonePosition.y && position.z >= zonePosition.y ||
			oldPosZ >= zonePosition.y && position.z < zonePosition.y)
		{
			load(executor, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}

		if (currentZoneX < zonesLengthX && currentZoneZ < zonesLengthZ && currentZoneX > 0u && currentZoneZ > 0u)
		{
			auto& currentZone = zones[currentZoneZ * zonesLengthX + currentZoneX];
			if (currentZone.changeArea(executor)) { return; } //we have entered a new area
		}

		oldPosX = position.x;
		oldPosZ = position.z;

		executor->updateJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor)
		{
			const auto ths = reinterpret_cast<CaveArea* const>(requester);
			ths->update(executor);
		}));
	}

	void CaveArea::setAsCurrentArea(BaseExecutor* const executor)
	{
		auto& position = executor->sharedResources->playerPosition.location.position;
		oldPosX = position.x;
		oldPosZ = position.z;

		currentZoneX = static_cast<int>((position.x - positionX) / (Area::zoneDiameter / 2.0f));
		currentZoneZ = static_cast<int>((position.z - positionZ) / (Area::zoneDiameter / 2.0f));

		executor->updateJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor)
		{
			const auto ths = reinterpret_cast<CaveArea* const>(requester);
			ths->update(executor);
		}));
	}

	void CaveArea::start(BaseExecutor* const executor)
	{
		Vector2 position{ currentZoneX * Area::zoneDiameter, currentZoneZ * Area::zoneDiameter };
		load(executor, position, 0.0f, nullptr);

		auto& playerPosition = executor->sharedResources->playerPosition.location.position;
		oldPosX = playerPosition.x;
		oldPosZ = playerPosition.z;

		executor->updateJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor)
		{
			const auto ths = reinterpret_cast<CaveArea* const>(requester);
			ths->update(executor);
		}));
	}

}