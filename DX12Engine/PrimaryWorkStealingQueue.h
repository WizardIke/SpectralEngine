#pragma once
#include "../WorkStealingQueue/WorkStealingQueue.h"
#include "Job.h"

using PrimaryWorkStealingQueue = WorkStealingStack<Job, 128u>;