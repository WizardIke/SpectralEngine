#pragma once
#include "Outside/OutSideArea.h"
#include "Cave/CaveArea.h"
class BaseExecutor;

class Areas
{
public:
	OutSideArea outside;
	Cave::CaveArea cave;

	Areas(BaseExecutor* executor);
};