#pragma once
#include <Area.h>
#include <Vector2.h>
#include <Zone.h>
class ThreadResources;
class GlobalResources;


namespace Cave
{

	class CaveArea
	{
		constexpr static float highDetailRadius = Area::zoneDiameter * 3.f;
		constexpr static float mediumDetailRadius = Area::zoneDiameter * 6.f;
		constexpr static float positionX = 0.0f, positionZ = 100.0f;

		constexpr static unsigned int zonesLengthX = 1u;
		constexpr static unsigned int zonesLengthZ = 1u;
		constexpr static unsigned int numZones = zonesLengthX * zonesLengthZ;

		Zone<ThreadResources, GlobalResources> zones[numZones];

		int currentZoneX, currentZoneZ;
		float oldPosX, oldPosZ;

		void update(ThreadResources& threadResources, GlobalResources& globalResources);
	public:
		CaveArea();

		void start(ThreadResources& threadResources, GlobalResources& globalResources);
		void setAsCurrentArea(ThreadResources& threadResources, GlobalResources& globalResources);
		void load(ThreadResources& threadResources, GlobalResources& globalResources, Vector2 position, float distance, Area::VisitedNode* loadedAreas);
	};

}
