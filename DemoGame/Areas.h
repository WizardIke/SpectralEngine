#pragma once
#include <Zone.h>
#include <World.h>
#include <WorldManager.h>
#include <Vector3.h>
class ThreadResources;

class Areas
{
	constexpr static unsigned int levelOfDetailMaxDistancesCount = 3u;
	constexpr static float levelOfDetailMaxDistances[levelOfDetailMaxDistancesCount] = {384.0f, 768.0f, 4096.0f};
public:
	Zone<ThreadResources> zones[111];
	const World worlds[2];
	WorldManager<ThreadResources> worldManager;
	using StopRequest = WorldManager<ThreadResources>::StopRequest;

	Areas(void* context);
	void start(ThreadResources& threadResources);
	void stop(StopRequest& stopRequest, ThreadResources& threadResources);
	void setPosition(const Vector3& position, unsigned long worldIndex);
};