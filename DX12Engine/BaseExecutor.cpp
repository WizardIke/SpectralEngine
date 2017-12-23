#include "BaseExecutor.h"
#include "HresultException.h"
#include "SharedResources.h"

BaseExecutor::BaseExecutor(SharedResources* const sharedResources) :
	sharedResources(sharedResources),
	currentWorkStealingDeque(&workStealDeques[1u])
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

void BaseExecutor::quit(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock)
{
	const auto sharedResources = exe->sharedResources;
	++sharedResources->numThreadsThatHaveFinished;
	if (sharedResources->numThreadsThatHaveFinished == sharedResources->maxThreads)
	{
		sharedResources->numThreadsThatHaveFinished = 0u;
		++sharedResources->generation;
	}
	else
	{
		sharedResources->conditionVariable.wait(lock, [oldGen = sharedResources->generation, &gen = sharedResources->generation]() {return oldGen != gen; });
	}
	lock.unlock();
	exe->mQuit = true;
}


void BaseExecutor::doPrimaryJob()
{
	Job job;
	bool found = currentWorkStealingDeque->pop(job);
	if (found)
	{
		job(this);
	}
	else
	{
		for (auto i = 0u;; ++i)
		{
			//steal from other thread
			found = sharedResources->currentWorkStealingQueues[i].steal(job);
			if (found)
			{
				job(this);
				break;
			}
			else if (i == sharedResources->maxThreads - 1u)
			{
				std::unique_lock<std::mutex> lock(sharedResources->syncMutex);
				sharedResources->nextPhaseJob(this, std::move(lock));
				break;
			}
		}
	}
}

void BaseExecutor::run()
{
	mQuit = false;
	while (!mQuit)
	{
		doPrimaryJob();
	}
}

void BaseExecutor::runBackgroundJobs(Job job)
{
	bool found;
	do
	{
		job(this);
		found = sharedResources->backgroundQueue.pop(job);
	} while (found);
}