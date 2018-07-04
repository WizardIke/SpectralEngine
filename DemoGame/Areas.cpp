#include "Areas.h"
#include "ThreadResources.h"
#include "GlobalResources.h"

Areas::Areas(ThreadResources& executor, GlobalResources& sharedResources)
{
	outside.start(executor, sharedResources);
}