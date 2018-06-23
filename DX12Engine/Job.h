#pragma once
#include "Delegate.h"
class BaseExecutor;
class SharedResources;

using Job = Delegate<void(BaseExecutor* executor, SharedResources& sharedResources)>;