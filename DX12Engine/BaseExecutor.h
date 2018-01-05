#pragma once

#include <d3d12.h>
#include "frameBufferCount.h"
#include "Frustum.h"
#include "../WorkStealingQueue/WorkStealingQueue.h"
#include "../LocklessQueue/LocklessQueue.h"
#include "../Array/Array.h"
#include "D3D12CommandAllocator.h"
#include "D3D12GraphicsCommandList.h"
#include "D3D12DescriptorHeap.h"
#include "StreamingManager.h"
#include "VRamFreeingManager.h"
#include "Job.h"
#include "FixedSizeAllocator.h"
#include <random>
#include <memory>
#include <mutex>
class SharedResources;

class BaseExecutor
{
protected:
	constexpr static unsigned int startingWorkStealingStackSize = 128u;
	WorkStealingStack<Job, startingWorkStealingStackSize> workStealDeques[2u];
	WorkStealingStack<Job, startingWorkStealingStackSize>* currentWorkStealingDeque;
	bool mQuit;//local


	void doPrimaryJob();
	void runBackgroundJobs(Job job);

#ifndef NDEBUG
	std::string type;
#endif // NDEBUG

	BaseExecutor(SharedResources* const sharedResources);
	~BaseExecutor();

	virtual void update2(std::unique_lock<std::mutex>&& lock) = 0;
public:
	SharedResources* sharedResources;//shared
	VRamFreeingManager vRamFreeingManager;
	std::default_random_engine randomNumberGenerator;
	FixedSizeAllocator<Mesh> meshAllocator;

	void run();

	WorkStealingStack<Job, startingWorkStealingStackSize>& renderJobQueue() { return workStealDeques[1u]; }
	WorkStealingStack<Job, startingWorkStealingStackSize>& updateJobQueue() { return workStealDeques[0u]; }
	WorkStealingStack<Job, startingWorkStealingStackSize>& initializeJobQueue() { return workStealDeques[1u]; }


	static void quit(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock);

	template<void(*update1NextPhaseJob)(BaseExecutor*, std::unique_lock<std::mutex>&& lock)>
	void initialize(std::unique_lock<std::mutex>&& lock, StreamingManagerThreadLocal& streamingManager)
	{
		if (streamingManager.hasPendingUploads())
		{
			++sharedResources->numThreadsThatHaveFinished;
			if (sharedResources->numThreadsThatHaveFinished % sharedResources->maxThreads == 0u)
			{
				sharedResources->numThreadsThatHaveFinished = 0u;
				++(sharedResources->generation);
				lock.unlock();
				sharedResources->conditionVariable.notify_all();
			}
			else
			{
				sharedResources->conditionVariable.wait(lock, [&gen = sharedResources->generation, oldGen = sharedResources->generation]() {return oldGen != gen; });
				lock.unlock();
			}

			streamingManager.update(this);
		}
		else
		{
			sharedResources->numThreadsThatHaveFinished += sharedResources->maxThreads + 1u;
			if (sharedResources->numThreadsThatHaveFinished % sharedResources->maxThreads == 0u)
			{
				if (sharedResources->numThreadsThatHaveFinished / sharedResources->maxThreads == sharedResources->maxThreads + 1u)
				{
					sharedResources->nextPhaseJob = update1NextPhaseJob;

					sharedResources->currentWorkStealingQueues = &sharedResources->workStealingQueues[0u];
					currentWorkStealingDeque = &workStealDeques[0u];
				}

				sharedResources->numThreadsThatHaveFinished = 0u;
				++(sharedResources->generation);
				lock.unlock();
				sharedResources->conditionVariable.notify_all();
			}
			else
			{
				sharedResources->conditionVariable.wait(lock, [&gen = sharedResources->generation, oldGen = sharedResources->generation]() {return oldGen != gen; });
				if (sharedResources->currentWorkStealingQueues == &sharedResources->workStealingQueues[0u])
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