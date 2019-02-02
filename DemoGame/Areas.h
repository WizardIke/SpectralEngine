#pragma once
#include "Outside/OutSideArea.h"
#include "Cave/CaveArea.h"
class ThreadResources;
class GlobalResources;

class Areas
{
public:
	OutSideArea outside;
	Cave::CaveArea cave;

	Areas();
	void start(ThreadResources& executor, GlobalResources& sharedResources);
};