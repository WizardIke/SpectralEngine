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
#include "MeshAllocator.h"
#include <random>
#include <memory>
#include <mutex>
class SharedResources;

class BaseExecutor
{
protected:
	constexpr static unsigned int startingWorkStealingStackSize = 128u;
	WorkStealingStack<Job, startingWorkStealingStackSize> workStealDeques[2u];


	void DoPrimaryJob();
	void runBackgroundJobs(Job job);
	void swapWorkStealingDeques()
	{
		if (currentWorkStealingDeque == &workStealDeques[0u])
		{
			currentWorkStealingDeque = &workStealDeques[1u];
		}
		else
		{
			currentWorkStealingDeque = &workStealDeques[0u];
		}
	}

#ifdef _DEBUG
	std::string type;
#endif // _DEBUG

	BaseExecutor(SharedResources* const sharedResources, unsigned long uploadHeapStartingSize, unsigned int uploadRequestBufferStartingCapacity, unsigned int halfFinishedUploadRequestBufferStartingCapasity);
	~BaseExecutor();

	virtual void update2(std::unique_lock<std::mutex>&& lock) = 0;

	WorkStealingStack<Job, startingWorkStealingStackSize>* currentWorkStealingDeque;
public:

	SharedResources* sharedResources;//shared
	StreamingManagerThreadLocal streamingManager;//local
	VRamFreeingManager vRamFreeingManager;
	std::default_random_engine randomNumberGenerator;
	FixedSizeAllocator meshAllocator;
	bool quit;//local

	void run();

	WorkStealingStack<Job, startingWorkStealingStackSize>& renderJobQueue() { return workStealDeques[1u]; }
	WorkStealingStack<Job, startingWorkStealingStackSize>& updateJobQueue() { return workStealDeques[0u]; }
	WorkStealingStack<Job, startingWorkStealingStackSize>& initializeJobQueue() { return workStealDeques[1u]; }


	template<void(*update1NextPhaseJob)(BaseExecutor*, std::unique_lock<std::mutex>&& lock)>
	void initialize(std::unique_lock<std::mutex>&& lock)
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
					std::swap(sharedResources->currentWorkStealingQueues, sharedResources->nextWorkStealingQueues);
					sharedResources->nextPhaseJob = update1NextPhaseJob;

					swapWorkStealingDeques();
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