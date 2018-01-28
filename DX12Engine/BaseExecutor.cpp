#include "BaseExecutor.h"
#include "HresultException.h"
#include "SharedResources.h"

BaseExecutor::BaseExecutor(SharedResources* const sharedResources) :
	currentWorkStealingDeque(&workStealDeques[1u]),
	gpuCompletionEventManager(),
	randomNumberGenerator(),
	meshAllocator()
{
	sharedResources->workStealingQueues[sharedResources->numThreadsThatHaveFinished] = workStealDeques[0u];
	sharedResources->workStealingQueues[sharedResources->numThreadsThatHaveFinished + sharedResources->maxThreads] = workStealDeques[1u];
	++sharedResources->numThreadsThatHaveFinished;
	if (sharedResources->numThreadsThatHaveFinished == sharedResources->maxThreads)
	{
		sharedResources->numThreadsThatHaveFinished = 0u;
	}
}

BaseExecutor::~BaseExecutor() {}

void BaseExecutor::quit(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock)
{
	++sharedResources.numThreadsThatHaveFinished;
	if (sharedResources.numThreadsThatHaveFinished == sharedResources.maxThreads)
	{
		sharedResources.numThreadsThatHaveFinished = 0u;
		++sharedResources.generation;
	}
	else
	{
		sharedResources.conditionVariable.wait(lock, [oldGen = sharedResources.generation, &gen = sharedResources.generation]() {return oldGen != gen; });
	}
	lock.unlock();
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
			found = sharedResources.currentWorkStealingQueues[i].steal(job);
			if (found)
			{
				job(this, sharedResources);
				break;
			}
			else if (i == sharedResources.maxThreads - 1u)
			{
				std::unique_lock<std::mutex> lock(sharedResources.syncMutex);
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