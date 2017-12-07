#pragma once
#include <Area.h>
#include <Vector2.h>
#include <basezone.h>
class BaseExecutor;
class SharedResources;


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

		BaseZone zones[numZones];

		int currentZoneX, currentZoneZ;
		float oldPosX, oldPosZ;

		void update(BaseExecutor* const executor);
	public:
		CaveArea();

		void start(BaseExecutor* const executor);
		void setAsCurrentArea(BaseExecutor* const executor);
		void load(BaseExecutor* const executor, Vector2 position, float distance, Area::VisitedNode* loadedAreas);
	};

}
