#include "BackgroundExecutor.h"
#include "Assets.h"
#include <D3D12GraphicsEngine.h>

BackgroundExecutor::BackgroundExecutor(SharedResources& sharedResources) : Executor(&sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
{
#ifndef NDEBUG
	type = "BackgroundExecutor";
#endif // NDEBUG
}

void BackgroundExecutor::run(SharedResources& sharedResources)
{
	thread = std::thread([](std::pair<Executor*, SharedResources*> data) {data.first->run(*data.second); }, std::make_pair(static_cast<Executor*const>(this), &sharedResources));
}

void BackgroundExecutor::update2(std::unique_lock<std::mutex>&& lock, SharedResources& sr)
{
	Assets& sharedResources = reinterpret_cast<Assets&>(sr);
	renderPass.update2(this, sharedResources, sharedResources.renderPass);
	++(sharedResources.numThreadsThatHaveFinished);

	if (sharedResources.numThreadsThatHaveFinished == sharedResources.maxPrimaryThreads + sharedResources.numPrimaryJobExeThreads)
	{
		sharedResources.conditionVariable.notify_all();
	}

	sharedResources.conditionVariable.wait(lock, [&generation = sharedResources.generation, gen = sharedResources.generation]() {return gen != generation; });

	Job job;
	bool found = sharedResources.backgroundQueue.pop(job);
	if (found)
	{
		--(sharedResources.numPrimaryJobExeThreads);
		lock.unlock();

		gpuCompletionEventManager.update(this, sharedResources);
		streamingManager.update(this, sharedResources);

		Executor::runBackgroundJobs(job, sharedResources);
		getIntoCorrectStateAfterDoingBackgroundJob(sharedResources);
	}
	else
	{
		lock.unlock();
		currentWorkStealingDeque = &updateJobQueue();
		gpuCompletionEventManager.update(this, sharedResources);
		streamingManager.update(this, sharedResources);
	}
}

void BackgroundExecutor::getIntoCorrectStateAfterDoingBackgroundJob(SharedResources& sharedResources)
{
	const auto assets = reinterpret_cast<Assets*>(&sharedResources);
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

			renderPass.update1After(*assets, assets->renderPass, assets->rootSignatures.rootSignature, 1u);
		}
		
	} 
	else
	{
		++(assets->numPrimaryJobExeThreads);

		currentWorkStealingDeque = &updateJobQueue();
		lock.unlock();
	}
}