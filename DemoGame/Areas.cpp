#include "Areas.h"
#include "ThreadResources.h"
#include "GlobalResources.h"

Areas::Areas() {}

void Areas::start(ThreadResources& executor, GlobalResources& sharedResources)
{
	outside.start(executor, sharedResources);
}