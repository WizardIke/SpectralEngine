#pragma once
#include <Area.h>
#include <Zone.h>
#include <Vector2.h>
class ThreadResources;
class GlobalResources;

class OutSideArea
{
	constexpr static float highDetailRadius = Area::zoneDiameter * 3.f;
	constexpr static float mediumDetailRadius = Area::zoneDiameter * 6.f;
	constexpr static float lowDetailRadius = Area::zoneDiameter * 32.f;

	constexpr static unsigned int zonesLengthX = 10u;
	constexpr static unsigned int zonesLengthZ = 11u;
	constexpr static unsigned int numZones = zonesLengthX * zonesLengthZ;

	Zone<ThreadResources, GlobalResources> zones[numZones];

	int currentZoneX, currentZoneZ;
	float oldPosX, oldPosZ;

	void update(ThreadResources& executor, GlobalResources& sharedResources);
public:
	OutSideArea();

	void load(ThreadResources& executor, GlobalResources& sharedResources, Vector2 position, float distance, Area::VisitedNode* loadedAreas);
	void start(ThreadResources& executor, GlobalResources& sharedResources);
	void setAsCurrentArea(ThreadResources& executor, GlobalResources& sharedResources);
};