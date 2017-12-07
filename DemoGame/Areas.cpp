#include "Areas.h"
#include <BaseExecutor.h>

Areas::Areas(BaseExecutor* executor)
{
	outside.start(executor);
}