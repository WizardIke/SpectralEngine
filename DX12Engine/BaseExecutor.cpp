#include "BaseExecutor.h"
#include "HresultException.h"
#include "SharedResources.h"

BaseExecutor::BaseExecutor(SharedResources* const sharedResources, unsigned long uploadHeapStartingSize, unsigned int uploadRequestBufferStartingCapacity, unsigned int halfFinishedUploadRequestBufferStartingCapasity) :
	sharedResources(sharedResources),
	streamingManager(sharedResources->graphicsEngine.graphicsDevice, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity),
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


void BaseExecutor::DoPrimaryJob()
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
	quit = false;
	while (!quit)
	{
		DoPrimaryJob();
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