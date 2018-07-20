#include "CaveArea.h"
#include "../ThreadResources.h"
#include "../GlobalResources.h"
#include "CaveZone1.h"

namespace Cave
{

	CaveArea::CaveArea() : 
		zones
		{
			Zone1()
		}
	{}

	void CaveArea::load(ThreadResources& threadResources, GlobalResources& globalResources, Vector2 position, float distanceToAreaEntrance, Area::VisitedNode* loadedAreas)
	{
		Area::VisitedNode thisArea;
		thisArea.thisArea = this;
		thisArea.next = loadedAreas;
		while (loadedAreas != nullptr)
		{
			if (loadedAreas->thisArea == this) return; //this area has already been loaded
			loadedAreas = loadedAreas->next;
		}

		constexpr int numZonesRadius = Area::constexprCeil(mediumDetailRadius / Area::zoneDiameter);
		unsigned int minX, maxX, minZ, maxZ;
		minX = currentZoneX > numZonesRadius ? currentZoneX - numZonesRadius : 0u;
		maxX = currentZoneX + numZonesRadius < zonesLengthX - 1u ? currentZoneX + numZonesRadius : zonesLengthX - 1u;
		minZ = currentZoneZ > numZonesRadius ? currentZoneZ - numZonesRadius : 0u;
		maxZ = currentZoneZ + numZonesRadius < zonesLengthZ - 1u ? currentZoneZ + numZonesRadius : zonesLengthZ - 1u;

		float currentHighDetailRadius = highDetailRadius - distanceToAreaEntrance;
		float currentMediumDetailRadius = mediumDetailRadius - distanceToAreaEntrance;
		if (currentMediumDetailRadius <= 0.0f) return;

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
				float distanceFromAreaEntrance = std::sqrt(difX * difX + difZ * difZ);

				if (distanceFromAreaEntrance < currentHighDetailRadius)
				{
					zone.setState(0u, threadResources, globalResources);
					zone.loadConnectedAreas(threadResources, globalResources, distanceFromAreaEntrance + distanceToAreaEntrance, &thisArea);
				}
				else if (distanceFromAreaEntrance < currentMediumDetailRadius)
				{
					zone.setState(1u, threadResources, globalResources);
					zone.loadConnectedAreas(threadResources, globalResources, distanceFromAreaEntrance + distanceToAreaEntrance, &thisArea);
				}
				else
				{
					zone.setState(2u, threadResources, globalResources);
					zone.loadConnectedAreas(threadResources, globalResources, distanceFromAreaEntrance + distanceToAreaEntrance, &thisArea);
				}
			}
		}
	}

	void CaveArea::update(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		auto& position = globalResources.mainCamera().position();
		Vector2 zonePosition{ currentZoneX * Area::zoneDiameter + 0.5f * Area::zoneDiameter, currentZoneZ * Area::zoneDiameter + 0.5f * Area::zoneDiameter };
		if (position.x > zonePosition.x + 0.5f * Area::zoneDiameter)
		{
			++currentZoneX;
			load(threadResources, globalResources, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (position.x < zonePosition.x - 0.5f * Area::zoneDiameter)
		{
			--currentZoneX;
			load(threadResources, globalResources, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (position.z > zonePosition.y + 0.5f * Area::zoneDiameter)
		{
			++currentZoneZ;
			load(threadResources, globalResources, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (position.z < zonePosition.y - 0.5f * Area::zoneDiameter)
		{
			--currentZoneZ;
			load(threadResources, globalResources, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}
		else if (oldPosX < zonePosition.x && position.x >= zonePosition.x ||
			oldPosX >= zonePosition.x && position.x < zonePosition.x ||
			oldPosZ < zonePosition.y && position.z >= zonePosition.y ||
			oldPosZ >= zonePosition.y && position.z < zonePosition.y)
		{
			load(threadResources, globalResources, Vector2{ zonePosition.x - 0.5f * Area::zoneDiameter, zonePosition.y - 0.5f * Area::zoneDiameter }, 0.0f, nullptr);
		}

		if (currentZoneX < zonesLengthX && currentZoneZ < zonesLengthZ && currentZoneX > 0u && currentZoneZ > 0u)
		{
			auto& currentZone = zones[currentZoneZ * zonesLengthX + currentZoneX];
			if (currentZone.changeArea(threadResources, globalResources)) { return; } //we have entered a new area
		}

		oldPosX = position.x;
		oldPosZ = position.z;

		threadResources.taskShedular.update1NextQueue().push({ this, [](void*const requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			const auto ths = reinterpret_cast<CaveArea* const>(requester);
			ths->update(threadResources, globalResources);
		} });
	}

	void CaveArea::setAsCurrentArea(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		auto& position = globalResources.mainCamera().position();
		oldPosX = position.x;
		oldPosZ = position.z;

		currentZoneX = static_cast<int>((position.x - positionX) / (Area::zoneDiameter / 2.0f));
		currentZoneZ = static_cast<int>((position.z - positionZ) / (Area::zoneDiameter / 2.0f));

		threadResources.taskShedular.update1NextQueue().push({ this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			const auto ths = reinterpret_cast<CaveArea* const>(requester);
			ths->update(threadResources, globalResources);
		} });
	}

	void CaveArea::start(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		Vector2 position{ currentZoneX * Area::zoneDiameter, currentZoneZ * Area::zoneDiameter };
		load(threadResources, globalResources, position, 0.0f, nullptr);

		auto& cameraPosition = globalResources.mainCamera().position();
		oldPosX = cameraPosition.x;
		oldPosZ = cameraPosition.z;

		threadResources.taskShedular.update1NextQueue().push({ this, [](void*const requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			const auto ths = reinterpret_cast<CaveArea* const>(requester);
			ths->update(threadResources, globalResources);
		} });
	}

}