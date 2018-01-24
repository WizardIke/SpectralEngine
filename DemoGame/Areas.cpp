#include "Areas.h"
#include <BaseExecutor.h>
#include <SharedResources.h>

Areas::Areas(BaseExecutor* executor, SharedResources& sharedResources)
{
	outside.start(executor, sharedResources);
}