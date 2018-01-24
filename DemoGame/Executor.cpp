#include "Executor.h"
#include <BaseExecutor.h>
#include "Assets.h"

void Executor::update1NextPhaseJob(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock)
{
	Executor* executor = (Executor*)exe;
	executor->update1(std::move(lock), sharedResources);
}

void Executor::initialize(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock)
{
	auto executor = reinterpret_cast<Executor*>(exe);
	exe->initialize<Executor::update1NextPhaseJob>(std::move(lock), executor->streamingManager, sharedResources);
}

void Executor::update1(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources)
{
	Assets* const assets = (Assets*)&sharedResources;
	unsigned int oldIndex = assets->numThreadsThatHaveFinished;
	renderPass.update1(this, assets->renderPass, oldIndex);

	if (assets->numThreadsThatHaveFinished == assets->maxPrimaryThreads + assets->numPrimaryJobExeThreads)
	{
		assets->renderPass.update1(assets->graphicsEngine);
		assets->currentWorkStealingQueues = &assets->workStealingQueues[assets->maxThreads];

		assets->nextPhaseJob = update2NextPhaseJob;
		assets->numThreadsThatHaveFinished = 0u;
		++(assets->generation);
		lock.unlock();
		assets->conditionVariable.notify_all();
	}
	else
	{
		++(assets->numThreadsThatHaveFinished);
		assets->conditionVariable.wait(lock, [&generation = assets->generation, gen = assets->generation]() {return gen != generation; });
		lock.unlock();
	}
	
	currentWorkStealingDeque = &workStealDeques[1u];
	renderPass.update1After(*assets, assets->renderPass, assets->rootSignatures.rootSignature, oldIndex);
}

void Executor::update2(std::unique_lock<std::mutex>&& lock, SharedResources& sr)
{
	Assets& sharedResources = reinterpret_cast<Assets&>(sr);
	renderPass.update2(this, sharedResources, sharedResources.renderPass);
	++(sharedResources.numThreadsThatHaveFinished);

	if (sharedResources.numThreadsThatHaveFinished == sharedResources.maxPrimaryThreads + sharedResources.numPrimaryJobExeThreads)
	{
		sharedResources.conditionVariable.notify_all();
	}

	sharedResources.conditionVariable.wait(lock, [&generation = sharedResources.generation, gen = sharedResources.generation]() {return gen != generation; });
	lock.unlock();

	currentWorkStealingDeque = &workStealDeques[0u];
	gpuCompletionEventManager.update(this, sharedResources);
	streamingManager.update(this, sharedResources);
}