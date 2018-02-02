#include "MainExecutor.h"
#include "Assets.h"

MainExecutor::MainExecutor(SharedResources& sharedResources) :
	Executor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
{
#ifdef _DEBUG
	this->type = "MainExecutor";
#endif // _DEBUG
}

void MainExecutor::update2(std::unique_lock<std::mutex>&& lock, SharedResources& sr)
{
	Assets& sharedResources = (Assets&)sr;
	sharedResources.threadBarrier.wait(lock, [&assets = sharedResources]() {return assets.threadBarrier.waiting_count() == assets.maxPrimaryThreads + assets.numPrimaryJobExeThreads; });

	sharedResources.nextPhaseJob = update1NextPhaseJob;
	renderPass.update2LastThread(this, sharedResources, sharedResources.renderPass);
	sharedResources.currentWorkStealingQueues = &sharedResources.workStealingQueues[0u];
	sharedResources.update(this);
	sharedResources.threadBarrier.waiting_count() = 0u;
	++(sharedResources.threadBarrier.generation());
	lock.unlock();
	sharedResources.threadBarrier.notify_all();

	currentWorkStealingDeque = &workStealDeques[0u];
	gpuCompletionEventManager.update(this, sharedResources);
	streamingManager.update(this, sharedResources);
}