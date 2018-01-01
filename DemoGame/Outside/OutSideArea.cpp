#include "OutSideArea.h"
#include <BaseExecutor.h>
#include <SharedResources.h>
#include "zone1.h"
#include "../TestZone.h"

OutSideArea::OutSideArea() :
	currentZoneX(0u), currentZoneZ(0u),
	zones
	{ 
		Zone1(),			nullptr,			nullptr,			nullptr,			nullptr,			nullptr,			nullptr,			nullptr,			nullptr,			nullptr,
		TestZone<0u, 0u>(), TestZone<1u, 0u>(), TestZone<2u, 0u>(), TestZone<3u, 0u>(), TestZone<4u, 0u>(), TestZone<5u, 0u>(), TestZone<6u, 0u>(), TestZone<7u, 0u>(), TestZone<8u, 0u>(), TestZone<9u, 0u>(),
		TestZone<0u, 1u>(), TestZone<1u, 1u>(), TestZone<2u, 1u>(), TestZone<3u, 1u>(), TestZone<4u, 1u>(), TestZone<5u, 1u>(), TestZone<6u, 1u>(), TestZone<7u, 1u>(), TestZone<8u, 1u>(), TestZone<9u, 1u>(),
		TestZone<0u, 2u>(), TestZone<1u, 2u>(), TestZone<2u, 2u>(), TestZone<3u, 2u>(), TestZone<4u, 2u>(), TestZone<5u, 2u>(), TestZone<6u, 2u>(), TestZone<7u, 2u>(), TestZone<8u, 2u>(), TestZone<9u, 2u>(),
		TestZone<0u, 3u>(), TestZone<1u, 3u>(), TestZone<2u, 3u>(), TestZone<3u, 3u>(), TestZone<4u, 3u>(), TestZone<5u, 3u>(), TestZone<6u, 3u>(), TestZone<7u, 3u>(), TestZone<8u, 3u>(), TestZone<9u, 3u>(),
		TestZone<0u, 4u>(), TestZone<1u, 4u>(), TestZone<2u, 4u>(), TestZone<3u, 4u>(), TestZone<4u, 4u>(), TestZone<5u, 4u>(), TestZone<6u, 4u>(), TestZone<7u, 4u>(), TestZone<8u, 4u>(), TestZone<9u, 4u>(),
		TestZone<0u, 5u>(), TestZone<1u, 5u>(), TestZone<2u, 5u>(), TestZone<3u, 5u>(), TestZone<4u, 5u>(), TestZone<5u, 5u>(), TestZone<6u, 5u>(), TestZone<7u, 5u>(), TestZone<8u, 5u>(), TestZone<9u, 5u>(),
		TestZone<0u, 6u>(), TestZone<1u, 6u>(), TestZone<2u, 6u>(), TestZone<3u, 6u>(), TestZone<4u, 6u>(), TestZone<5u, 6u>(), TestZone<6u, 6u>(), TestZone<7u, 6u>(), TestZone<8u, 6u>(), TestZone<9u, 6u>(),
		TestZone<0u, 7u>(), TestZone<1u, 7u>(), TestZone<2u, 7u>(), TestZone<3u, 7u>(), TestZone<4u, 7u>(), TestZone<5u, 7u>(), TestZone<6u, 7u>(), TestZone<7u, 7u>(), TestZone<8u, 7u>(), TestZone<9u, 7u>(),
		TestZone<0u, 8u>(), TestZone<1u, 8u>(), TestZone<2u, 8u>(), TestZone<3u, 8u>(), TestZone<4u, 8u>(), TestZone<5u, 8u>(), TestZone<6u, 8u>(), TestZone<7u, 8u>(), TestZone<8u, 8u>(), TestZone<9u, 8u>(),
		TestZone<0u, 9u>(), TestZone<1u, 9u>(), TestZone<2u, 9u>(), TestZone<3u, 9u>(), TestZone<4u, 9u>(), TestZone<5u, 9u>(), TestZone<6u, 9u>(), TestZone<7u, 9u>(), TestZone<8u, 9u>(), TestZone<9u, 9u>()
	} {}

void OutSideArea::load(BaseExecutor* const executor, Vector2 position, float distance, Area::VisitedNode* loadedAreas)
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

	constexpr int numZonesRadius = Area::constexprCeil(lowDetailRadius / Area::zoneDiameter);
	unsigned int minX, maxX, minZ, maxZ;
	minX = currentZoneX > numZonesRadius ? currentZoneX - numZonesRadius : 0u;
	maxX = currentZoneX + numZonesRadius < zonesLengthX - 1u ? currentZoneX + numZonesRadius : zonesLengthX - 1u;
	minZ = currentZoneZ > numZonesRadius ? currentZoneZ - numZonesRadius : 0u;
	maxZ = currentZoneZ + numZonesRadius < zonesLengthZ - 1u ? currentZoneZ + numZonesRadius : zonesLengthZ - 1u;

	float highDetailRadiusSquared = highDetailRadius - distance;
	highDetailRadiusSquared *= highDetailRadiusSquared;

	float mediumDetailRadiusSquared = mediumDetailRadius - distance;
	mediumDetailRadiusSquared *= mediumDetailRadiusSquared;

	float lowDetailRadiusSquared = lowDetailRadius - distance;
	lowDetailRadiusSquared *= lowDetailRadiusSquared;

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
			else if (distanceSquared < lowDetailRadiusSquared)
			{
				zone.loadLowDetail(executor);
			}
			else
			{
				zone.unloadAll(executor);
			}
		}
	}
}

void OutSideArea::update(BaseExecutor* const executor)
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

	executor->renderJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor)
	{
		executor->updateJobQueue().push(Job(requester, [](void*const requester, BaseExecutor*const executor)
		{
			const auto ths = reinterpret_cast<OutSideArea* const>(requester);
			ths->update(executor);
		}));
	}));
}

void OutSideArea::setAsCurrentArea(BaseExecutor* const executor)
{
	auto& position = executor->sharedResources->playerPosition.location.position;
	oldPosX = position.x;
	oldPosZ = position.z;

	currentZoneX = static_cast<int>(position.x / (Area::zoneDiameter / 2.0f));
	currentZoneZ = static_cast<int>(position.z / (Area::zoneDiameter / 2.0f));

	executor->renderJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor)
	{
		executor->updateJobQueue().push(Job(requester, [](void*const requester, BaseExecutor*const executor)
		{
			const auto ths = reinterpret_cast<OutSideArea* const>(requester);
			ths->update(executor);
		}));
	}));
}

void OutSideArea::start(BaseExecutor* const executor)
{
	Vector2 position{ currentZoneX * Area::zoneDiameter, currentZoneZ * Area::zoneDiameter };
	
	load(executor, position, 0.0f, nullptr);

	auto& playerPosition = executor->sharedResources->playerPosition.location.position;
	oldPosX = playerPosition.x;
	oldPosZ = playerPosition.z;

	executor->updateJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor)
	{
		const auto ths = reinterpret_cast<OutSideArea* const>(requester);
		ths->update(executor);
	}));
}