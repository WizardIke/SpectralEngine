#include "BackgroundExecutor.h"
#include "Assets.h"
#include <D3D12GraphicsEngine.h>

BackgroundExecutor::BackgroundExecutor(SharedResources*const sharedResources) : Executor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
{
#ifndef NDEBUG
	type = "BackgroundExecutor";
#endif // NDEBUG
}

void BackgroundExecutor::run()
{
	thread = std::thread([](Executor* exe) {exe->run(); }, static_cast<Executor*const>(this));
}

void BackgroundExecutor::update2(std::unique_lock<std::mutex>&& lock)
{
	renderPass.update2(this, ((Assets*)sharedResources)->renderPass);
	++(sharedResources->numThreadsThatHaveFinished);

	if (sharedResources->numThreadsThatHaveFinished == sharedResources->maxPrimaryThreads + sharedResources->numPrimaryJobExeThreads)
	{
		sharedResources->conditionVariable.notify_all();
	}

	sharedResources->conditionVariable.wait(lock, [&generation = sharedResources->generation, gen = sharedResources->generation]() {return gen != generation; });

	Job job;
	bool found = sharedResources->backgroundQueue.pop(job);
	if (found)
	{
		--(sharedResources->numPrimaryJobExeThreads);
		lock.unlock();

		vRamFreeingManager.update(this);
		streamingManager.update(this);

		Executor::runBackgroundJobs(job);
		getIntoCorrectStateAfterDoingBackgroundJob();
	}
	else
	{
		lock.unlock();
		currentWorkStealingDeque = &updateJobQueue();
		vRamFreeingManager.update(this);
		streamingManager.update(this);
	}
}

void BackgroundExecutor::getIntoCorrectStateAfterDoingBackgroundJob()
{
	const auto assets = reinterpret_cast<Assets*>(sharedResources);
	std::unique_lock<decltype(assets->syncMutex)> lock(assets->syncMutex);

	if (assets->nextPhaseJob == update2NextPhaseJob)
	{
		if (assets->numThreadsThatHaveFinished != 0u)
		{
			assets->conditionVariable.wait(lock, [oldGen = assets->generation, &gen = assets->generation]() {return oldGen != gen; });

			++(assets->numPrimaryJobExeThreads);

			currentWorkStealingDeque = &updateJobQueue();
			lock.unlock();
		}
		else
		{
			++(assets->numPrimaryJobExeThreads);

			currentWorkStealingDeque = &renderJobQueue();
			lock.unlock();

			renderPass.update1After(this, assets->renderPass, assets->rootSignatures.rootSignature, 1u);
		}
		
	} 
	else
	{
		++(assets->numPrimaryJobExeThreads);

		currentWorkStealingDeque = &updateJobQueue();
		lock.unlock();
	}
}