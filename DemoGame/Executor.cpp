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
	exe->initialize<Executor::update1NextPhaseJob>(std::move(lock), sharedResources);
}

void Executor::update1(std::unique_lock<std::mutex>&& lock, SharedResources& sr)
{
	Assets& sharedResources = (Assets&)sr;
	unsigned int oldIndex = sharedResources.threadBarrier.waiting_count();
	renderPass.update1(this, sharedResources.renderPass, oldIndex);

	if (sharedResources.threadBarrier.waiting_count() == sharedResources.maxPrimaryThreads + sharedResources.numPrimaryJobExeThreads)
	{
		sharedResources.renderPass.update1(sharedResources.graphicsEngine);
		sharedResources.currentWorkStealingQueues = &sharedResources.workStealingQueues[sharedResources.maxThreads];

		sharedResources.nextPhaseJob = update2NextPhaseJob;
		sharedResources.threadBarrier.waiting_count() = 0u;
		++(sharedResources.threadBarrier.generation());
		lock.unlock();
		sharedResources.threadBarrier.notify_all();
	}
	else
	{
		++(sharedResources.threadBarrier.waiting_count());
		sharedResources.threadBarrier.wait(lock, [&generation = sharedResources.threadBarrier.generation(), gen = sharedResources.threadBarrier.generation()]() {return gen != generation; });
		lock.unlock();
	}
	
	currentWorkStealingDeque = &workStealDeques[1u];
	renderPass.update1After(sharedResources, sharedResources.renderPass, sharedResources.rootSignatures.rootSignature, oldIndex);
}

void Executor::update2(std::unique_lock<std::mutex>&& lock, SharedResources& sr)
{
	Assets& sharedResources = reinterpret_cast<Assets&>(sr);
	renderPass.update2(this, sharedResources, sharedResources.renderPass);
	++(sharedResources.threadBarrier.waiting_count());

	if (sharedResources.threadBarrier.waiting_count() == sharedResources.maxPrimaryThreads + sharedResources.numPrimaryJobExeThreads)
	{
		sharedResources.threadBarrier.notify_all();
	}

	sharedResources.threadBarrier.wait(lock, [&generation = sharedResources.threadBarrier.generation(), gen = sharedResources.threadBarrier.generation()]() {return gen != generation; });
	lock.unlock();

	currentWorkStealingDeque = &workStealDeques[0u];
	gpuCompletionEventManager.update(this, sharedResources);
	streamingManager.update(this, sharedResources);
}