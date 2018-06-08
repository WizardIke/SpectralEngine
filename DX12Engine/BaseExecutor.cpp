#include "BaseExecutor.h"
#include "HresultException.h"
#include "SharedResources.h"
#include <chrono>

BaseExecutor::BaseExecutor(SharedResources& sharedResources) :
	currentWorkStealingDeque(&workStealDeques[1u]),
	gpuCompletionEventManager(),
	randomNumberGenerator(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
	meshAllocator(),
	streamingManager(sharedResources.graphicsEngine.graphicsDevice)
{
	this->index = sharedResources.threadBarrier.waiting_count();
	sharedResources.executors[index] = this;
	sharedResources.workStealingQueues[index] = &workStealDeques[0u];
	sharedResources.workStealingQueues[index + sharedResources.maxThreads] = &workStealDeques[1u];
	++sharedResources.threadBarrier.waiting_count();
	if (sharedResources.threadBarrier.waiting_count() == sharedResources.maxThreads)
	{
		sharedResources.threadBarrier.waiting_count() = 0u;
	}

	streamingManager.update(this, sharedResources);
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

void BaseExecutor::runBackgroundJobs(SharedResources& sharedResources)
{
	bool found;
	Job job;
	IOCompletionPacket ioJob;
	while(true)
	{
		found = sharedResources.backgroundQueue.pop(job);
		if (found)
		{
			do
			{
				job(this, sharedResources);
				found = sharedResources.backgroundQueue.pop(job);
			} while (found);

			found = sharedResources.ioCompletionQueue.removeIOCompletionPacket(ioJob);
			while (found)
			{
				ioJob(this, sharedResources);
				found = sharedResources.ioCompletionQueue.removeIOCompletionPacket(ioJob);
			}
		}
		else
		{
			found = sharedResources.ioCompletionQueue.removeIOCompletionPacket(ioJob);
			if (found)
			{
				do
				{
					ioJob(this, sharedResources);
					found = sharedResources.ioCompletionQueue.removeIOCompletionPacket(ioJob);
				} while (found);
			}
			else
			{
				return;
			}
		}
	}
}