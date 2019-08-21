#pragma once
#include <Zone.h>
class ThreadResources;

namespace Cave
{
	Zone<ThreadResources> Zone1(void* context);
}