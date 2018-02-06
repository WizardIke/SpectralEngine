#include "BaseExecutor.h"
#include "HresultException.h"
#include "SharedResources.h"
#include <chrono>

BaseExecutor::BaseExecutor(SharedResources& sharedResources) :
	currentWorkStealingDeque(&workStealDeques[1u]),
	gpuCompletionEventManager(),
	randomNumberGenerator(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
	meshAllocator()
{
	sharedResources.workStealingQueues[sharedResources.threadBarrier.waiting_count()] = &workStealDeques[0u];
	sharedResources.workStealingQueues[sharedResources.threadBarrier.waiting_count() + sharedResources.maxThreads] = &workStealDeques[1u];
	++sharedResources.threadBarrier.waiting_count();
	if (sharedResources.threadBarrier.waiting_count() == sharedResources.maxThreads)
	{
		sharedResources.threadBarrier.waiting_count() = 0u;
	}
}

BaseExecutor::~BaseExecutor() {}

void BaseExecutor::quit(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock)
{
	sharedResources.threadBarrier.sync(std::move(lock), sharedResources.maxThreads);
	exe->mQuit = true;
}


void BaseExecutor::doPrimaryJob(SharedResources& sharedResources)
{
	Job job;
	bool found = currentWorkStealingDeque->pop(job);
	if (found)
	{
		job(this, sharedResources);
	}
	else
	{
		for (auto i = 0u;; ++i)
		{
			//steal from other thread
			found = sharedResources.currentWorkStealingQueues[i]->steal(job);
			if (found)
			{
				job(this, sharedResources);
				break;
			}
			else if (i == sharedResources.maxThreads - 1u)
			{
				std::unique_lock<std::mutex> lock(sharedResources.threadBarrier.mutex());
				sharedResources.nextPhaseJob(this, sharedResources, std::move(lock));
				break;
			}
		}
	}
}

void BaseExecutor::run(SharedResources& sharedResources)
{
	mQuit = false;
	while (!mQuit)
	{
		doPrimaryJob(sharedResources);
	}
}

void BaseExecutor::runBackgroundJobs(Job job, SharedResources& sharedResources)
{
	bool found;
	do
	{
		job(this, sharedResources);
		found = sharedResources.backgroundQueue.pop(job);
	} while (found);
}