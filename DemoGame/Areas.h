#pragma once
#include <Zone.h>
#include <World.h>
#include <WorldManager.h>
#include <Vector3.h>
class ThreadResources;
class GlobalResources;

class Areas
{
	constexpr static unsigned int levelOfDetailMaxDistancesCount = 3u;
	constexpr static float levelOfDetailMaxDistances[levelOfDetailMaxDistancesCount] = {384.0f, 768.0f, 4096.0f};
public:
	Zone<ThreadResources, GlobalResources> zones[111];
	const World worlds[2];
	WorldManager<ThreadResources, GlobalResources> worldManager;
	using StopRequest = WorldManager<ThreadResources, GlobalResources>::StopRequest;

	Areas();
	void start(ThreadResources& executor, GlobalResources& sharedResources);
	void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources& globalResources);
	void setPosition(const Vector3& position, unsigned long worldIndex);
};