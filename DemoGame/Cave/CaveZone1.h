#pragma once
#include <Zone.h>
class ThreadResources;
class GlobalResources;

namespace Cave
{
	Zone<ThreadResources, GlobalResources> Zone1();
}