#include "Areas.h"
#include "ThreadResources.h"
#include "GlobalResources.h"
#include "Outside/zone1.h"
#include "TestZone.h"
#include "Cave/CaveZone1.h"

Areas::Areas() :
	zones
	{
		Zone1(), TestZone<0u, 0u>(), TestZone<0u, 1u>(), TestZone<0u, 2u>(), TestZone<0u, 3u>(), TestZone<0u, 4u>(), TestZone<0u, 5u>(), TestZone<0u, 6u>(), TestZone<0u, 7u>(), TestZone<0u, 8u>(), TestZone<0u, 9u>(),
		nullptr, TestZone<1u, 0u>(), TestZone<1u, 1u>(), TestZone<1u, 2u>(), TestZone<1u, 3u>(), TestZone<1u, 4u>(), TestZone<1u, 5u>(), TestZone<1u, 6u>(), TestZone<1u, 7u>(), TestZone<1u, 8u>(), TestZone<1u, 9u>(),
		nullptr, TestZone<2u, 0u>(), TestZone<2u, 1u>(), TestZone<2u, 2u>(), TestZone<2u, 3u>(), TestZone<2u, 4u>(), TestZone<2u, 5u>(), TestZone<2u, 6u>(), TestZone<2u, 7u>(), TestZone<2u, 8u>(), TestZone<2u, 9u>(),
		nullptr, TestZone<3u, 0u>(), TestZone<3u, 1u>(), TestZone<3u, 2u>(), TestZone<3u, 3u>(), TestZone<3u, 4u>(), TestZone<3u, 5u>(), TestZone<3u, 6u>(), TestZone<3u, 7u>(), TestZone<3u, 8u>(), TestZone<3u, 9u>(),
		nullptr, TestZone<4u, 0u>(), TestZone<4u, 1u>(), TestZone<4u, 2u>(), TestZone<4u, 3u>(), TestZone<4u, 4u>(), TestZone<4u, 5u>(), TestZone<4u, 6u>(), TestZone<4u, 7u>(), TestZone<4u, 8u>(), TestZone<4u, 9u>(),
		nullptr, TestZone<5u, 0u>(), TestZone<5u, 1u>(), TestZone<5u, 2u>(), TestZone<5u, 3u>(), TestZone<5u, 4u>(), TestZone<5u, 5u>(), TestZone<5u, 6u>(), TestZone<5u, 7u>(), TestZone<5u, 8u>(), TestZone<5u, 9u>(),
		nullptr, TestZone<6u, 0u>(), TestZone<6u, 1u>(), TestZone<6u, 2u>(), TestZone<6u, 3u>(), TestZone<6u, 4u>(), TestZone<6u, 5u>(), TestZone<6u, 6u>(), TestZone<6u, 7u>(), TestZone<6u, 8u>(), TestZone<6u, 9u>(),
		nullptr, TestZone<7u, 0u>(), TestZone<7u, 1u>(), TestZone<7u, 2u>(), TestZone<7u, 3u>(), TestZone<7u, 4u>(), TestZone<7u, 5u>(), TestZone<7u, 6u>(), TestZone<7u, 7u>(), TestZone<7u, 8u>(), TestZone<7u, 9u>(),
		nullptr, TestZone<8u, 0u>(), TestZone<8u, 1u>(), TestZone<8u, 2u>(), TestZone<8u, 3u>(), TestZone<8u, 4u>(), TestZone<8u, 5u>(), TestZone<8u, 6u>(), TestZone<8u, 7u>(), TestZone<8u, 8u>(), TestZone<8u, 9u>(),
		nullptr, TestZone<9u, 0u>(), TestZone<9u, 1u>(), TestZone<9u, 2u>(), TestZone<9u, 3u>(), TestZone<9u, 4u>(), TestZone<9u, 5u>(), TestZone<9u, 6u>(), TestZone<9u, 7u>(), TestZone<9u, 8u>(), TestZone<9u, 9u>(),
		Cave::Zone1()
	},
	worlds
	{
		{10u, 1u, 11u, 0u},
		{1u, 1u, 1u, 110u}
	},
	worldManager(zones, worlds, levelOfDetailMaxDistances, levelOfDetailMaxDistancesCount)
{}

void Areas::start(ThreadResources& executor, GlobalResources& sharedResources)
{
	worldManager.start(executor, sharedResources);
}

void Areas::stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources& globalResources)
{
	worldManager.stop(stopRequest, threadResources, globalResources);
}

void Areas::setPosition(const Vector3& position, unsigned long worldIndex)
{
	worldManager.setPosition(position, worldIndex);
}