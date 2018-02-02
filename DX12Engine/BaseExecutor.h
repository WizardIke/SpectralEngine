#pragma once

#include <d3d12.h>
#include "frameBufferCount.h"
#include "../WorkStealingQueue/WorkStealingQueue.h"
#include "StreamingManager.h"
#include "GpuCompletionEventManager.h"
#include "Job.h"
#include "FixedSizeAllocator.h"
#undef min
#undef max
#include <pcg_random.hpp>
#include <random>
#include <memory>
#include <mutex>
class SharedResources;
#undef min
#undef max

class BaseExecutor
{
protected:
	constexpr static unsigned int startingWorkStealingStackSize = 128u;
	WorkStealingStack<Job, startingWorkStealingStackSize> workStealDeques[2u];
	WorkStealingStack<Job, startingWorkStealingStackSize>* currentWorkStealingDeque;
	bool mQuit;


	void doPrimaryJob(SharedResources& sharedResources);
	void runBackgroundJobs(Job job, SharedResources& sharedResources);

#ifndef NDEBUG
	std::string type;
#endif // NDEBUG

	BaseExecutor(SharedResources& sharedResources);
	~BaseExecutor();

	virtual void update2(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources) = 0;
public:
	GpuCompletionEventManager gpuCompletionEventManager;
	pcg32 randomNumberGenerator;
	FixedSizeAllocator<Mesh> meshAllocator;

	void run(SharedResources& sharedResources);

	WorkStealingStack<Job, startingWorkStealingStackSize>& renderJobQueue() { return workStealDeques[1u]; }
	WorkStealingStack<Job, startingWorkStealingStackSize>& updateJobQueue() { return workStealDeques[0u]; }
	WorkStealingStack<Job, startingWorkStealingStackSize>& initializeJobQueue() { return workStealDeques[1u]; }


	static void quit(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock);

	template<void(*update1NextPhaseJob)(BaseExecutor*, SharedResources&, std::unique_lock<std::mutex>&& lock)>
	void initialize(std::unique_lock<std::mutex>&& lock, StreamingManagerThreadLocal& streamingManager, SharedResources& sharedResources)
	{
		if (streamingManager.hasPendingUploads())
		{
			++sharedResources.threadBarrier.waiting_count();
			if (sharedResources.threadBarrier.waiting_count() % sharedResources.maxThreads == 0u)
			{
				sharedResources.threadBarrier.waiting_count() = 0u;
				++(sharedResources.threadBarrier.generation());
				lock.unlock();
				sharedResources.threadBarrier.notify_all();
			}
			else
			{
				sharedResources.threadBarrier.wait(lock, [&gen = sharedResources.threadBarrier.generation(), oldGen = sharedResources.threadBarrier.generation()]() {return oldGen != gen; });
				lock.unlock();
			}

			streamingManager.update(this, sharedResources);
		}
		else
		{
			sharedResources.threadBarrier.waiting_count() += sharedResources.maxThreads + 1u;
			if (sharedResources.threadBarrier.waiting_count() % sharedResources.maxThreads == 0u)
			{
				if (sharedResources.threadBarrier.waiting_count() / sharedResources.maxThreads == sharedResources.maxThreads + 1u)
				{
					sharedResources.nextPhaseJob = update1NextPhaseJob;

					sharedResources.currentWorkStealingQueues = &sharedResources.workStealingQueues[0u];
					currentWorkStealingDeque = &workStealDeques[0u];
				}

				sharedResources.threadBarrier.waiting_count() = 0u;
				++(sharedResources.threadBarrier.generation());
				lock.unlock();
				sharedResources.threadBarrier.notify_all();
			}
			else
			{
				sharedResources.threadBarrier.wait(lock, [&gen = sharedResources.threadBarrier.generation(), oldGen = sharedResources.threadBarrier.generation()]() {return oldGen != gen; });
				if (sharedResources.currentWorkStealingQueues == &sharedResources.workStealingQueues[0u])
				{
					currentWorkStealingDeque = &workStealDeques[0u];
				}
				else
				{
					currentWorkStealingDeque = &workStealDeques[1u];
				}
				lock.unlock();
			}
		}
	}
};