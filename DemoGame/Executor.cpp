#include "Executor.h"
#include <BaseExecutor.h>
#include "Assets.h"

void Executor::update1NextPhaseJob(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock)
{
	Executor* executor = (Executor*)exe;
	executor->update1(std::move(lock));
}

void Executor::update1(std::unique_lock<std::mutex>&& lock)
{
	Assets* const assets = (Assets*)sharedResources;
	unsigned int oldIndex = sharedResources->numThreadsThatHaveFinished;
	renderPass.update1(this, assets->renderPass, oldIndex);

	if (sharedResources->numThreadsThatHaveFinished == sharedResources->maxPrimaryThreads + sharedResources->numPrimaryJobExeThreads)
	{
		assets->renderPass.update1(assets->graphicsEngine);
		sharedResources->currentWorkStealingQueues = &sharedResources->workStealingQueues[sharedResources->maxThreads];

		sharedResources->nextPhaseJob = update2NextPhaseJob;
		sharedResources->numThreadsThatHaveFinished = 0u;
		++(sharedResources->generation);
		lock.unlock();
		sharedResources->conditionVariable.notify_all();
	}
	else
	{
		++(sharedResources->numThreadsThatHaveFinished);
		sharedResources->conditionVariable.wait(lock, [&generation = sharedResources->generation, gen = sharedResources->generation]() {return gen != generation; });
		lock.unlock();
	}
	
	currentWorkStealingDeque = &workStealDeques[1u];
	renderPass.update1After(this, assets->renderPass, assets->rootSignatures.rootSignature, oldIndex);
}

void Executor::update2(std::unique_lock<std::mutex>&& lock)
{
	renderPass.update2(this, ((Assets*)sharedResources)->renderPass);
	++(sharedResources->numThreadsThatHaveFinished);

	if (sharedResources->numThreadsThatHaveFinished == sharedResources->maxPrimaryThreads + sharedResources->numPrimaryJobExeThreads)
	{
		sharedResources->conditionVariable.notify_all();
	}

	sharedResources->conditionVariable.wait(lock, [&generation = sharedResources->generation, gen = sharedResources->generation]() {return gen != generation; });
	lock.unlock();

	currentWorkStealingDeque = &workStealDeques[0u];
	vRamFreeingManager.update(this);
	streamingManager.update(this);
}