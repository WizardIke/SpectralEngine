#pragma once
#include "Outside/OutSideArea.h"
#include "Cave/CaveArea.h"
class BaseExecutor;
class SharedResources;

class Areas
{
public:
	OutSideArea outside;
	Cave::CaveArea cave;

	Areas(BaseExecutor* executor, SharedResources& sharedResources);
};