#include "Areas.h"
#include "ThreadResources.h"
#include "zone1.h"
#include "TestZone.h"
#include "CaveZone1.h"

Areas::Areas(void* context) :
	zones
	{
		Zone1(context), TestZone<0u, 0u>(context), TestZone<0u, 1u>(context), TestZone<0u, 2u>(context), TestZone<0u, 3u>(context), TestZone<0u, 4u>(context), TestZone<0u, 5u>(context), TestZone<0u, 6u>(context), TestZone<0u, 7u>(context), TestZone<0u, 8u>(context), TestZone<0u, 9u>(context),
		nullptr, TestZone<1u, 0u>(context), TestZone<1u, 1u>(context), TestZone<1u, 2u>(context), TestZone<1u, 3u>(context), TestZone<1u, 4u>(context), TestZone<1u, 5u>(context), TestZone<1u, 6u>(context), TestZone<1u, 7u>(context), TestZone<1u, 8u>(context), TestZone<1u, 9u>(context),
		nullptr, TestZone<2u, 0u>(context), TestZone<2u, 1u>(context), TestZone<2u, 2u>(context), TestZone<2u, 3u>(context), TestZone<2u, 4u>(context), TestZone<2u, 5u>(context), TestZone<2u, 6u>(context), TestZone<2u, 7u>(context), TestZone<2u, 8u>(context), TestZone<2u, 9u>(context),
		nullptr, TestZone<3u, 0u>(context), TestZone<3u, 1u>(context), TestZone<3u, 2u>(context), TestZone<3u, 3u>(context), TestZone<3u, 4u>(context), TestZone<3u, 5u>(context), TestZone<3u, 6u>(context), TestZone<3u, 7u>(context), TestZone<3u, 8u>(context), TestZone<3u, 9u>(context),
		nullptr, TestZone<4u, 0u>(context), TestZone<4u, 1u>(context), TestZone<4u, 2u>(context), TestZone<4u, 3u>(context), TestZone<4u, 4u>(context), TestZone<4u, 5u>(context), TestZone<4u, 6u>(context), TestZone<4u, 7u>(context), TestZone<4u, 8u>(context), TestZone<4u, 9u>(context),
		nullptr, TestZone<5u, 0u>(context), TestZone<5u, 1u>(context), TestZone<5u, 2u>(context), TestZone<5u, 3u>(context), TestZone<5u, 4u>(context), TestZone<5u, 5u>(context), TestZone<5u, 6u>(context), TestZone<5u, 7u>(context), TestZone<5u, 8u>(context), TestZone<5u, 9u>(context),
		nullptr, TestZone<6u, 0u>(context), TestZone<6u, 1u>(context), TestZone<6u, 2u>(context), TestZone<6u, 3u>(context), TestZone<6u, 4u>(context), TestZone<6u, 5u>(context), TestZone<6u, 6u>(context), TestZone<6u, 7u>(context), TestZone<6u, 8u>(context), TestZone<6u, 9u>(context),
		nullptr, TestZone<7u, 0u>(context), TestZone<7u, 1u>(context), TestZone<7u, 2u>(context), TestZone<7u, 3u>(context), TestZone<7u, 4u>(context), TestZone<7u, 5u>(context), TestZone<7u, 6u>(context), TestZone<7u, 7u>(context), TestZone<7u, 8u>(context), TestZone<7u, 9u>(context),
		nullptr, TestZone<8u, 0u>(context), TestZone<8u, 1u>(context), TestZone<8u, 2u>(context), TestZone<8u, 3u>(context), TestZone<8u, 4u>(context), TestZone<8u, 5u>(context), TestZone<8u, 6u>(context), TestZone<8u, 7u>(context), TestZone<8u, 8u>(context), TestZone<8u, 9u>(context),
		nullptr, TestZone<9u, 0u>(context), TestZone<9u, 1u>(context), TestZone<9u, 2u>(context), TestZone<9u, 3u>(context), TestZone<9u, 4u>(context), TestZone<9u, 5u>(context), TestZone<9u, 6u>(context), TestZone<9u, 7u>(context), TestZone<9u, 8u>(context), TestZone<9u, 9u>(context),
		Cave::Zone1(context)
	},
	worlds
	{
		{10u, 1u, 11u, 0u},
		{1u, 1u, 1u, 110u}
	},
	worldManager(zones, worlds, levelOfDetailMaxDistances, levelOfDetailMaxDistancesCount)
{}

void Areas::start(ThreadResources& threadResources)
{
	worldManager.start(threadResources);
}

void Areas::stop(StopRequest& stopRequest, ThreadResources& threadResources)
{
	worldManager.stop(stopRequest, threadResources);
}

void Areas::setPosition(const Vector3& position, unsigned long worldIndex)
{
	worldManager.setPosition(position, worldIndex);
}