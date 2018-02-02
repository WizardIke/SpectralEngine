#pragma once
#include <Area.h>
#include <BaseZone.h>
#include <Vector2.h>
class BaseExecutor;
class SharedResources;

class OutSideArea
{
	constexpr static float highDetailRadius = Area::zoneDiameter * 3.f;
	constexpr static float mediumDetailRadius = Area::zoneDiameter * 6.f;
	constexpr static float lowDetailRadius = Area::zoneDiameter * 32.f;

	constexpr static unsigned int zonesLengthX = 10u;
	constexpr static unsigned int zonesLengthZ = 11u;
	constexpr static unsigned int numZones = zonesLengthX * zonesLengthZ;

	BaseZone zones[numZones];

	int currentZoneX, currentZoneZ;
	float oldPosX, oldPosZ;

	void update(BaseExecutor* const executor, SharedResources& sharedResources);
public:
	OutSideArea();

	void load(BaseExecutor* const executor, SharedResources& sharedResources, Vector2 position, float distance, Area::VisitedNode* loadedAreas);
	void start(BaseExecutor* const executor, SharedResources& sharedResources);
	void setAsCurrentArea(BaseExecutor* const executor, SharedResources& sharedResources);
};