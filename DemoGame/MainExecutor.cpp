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
	sharedResources.conditionVariable.wait(lock, [&assets = sharedResources]() {return assets.numThreadsThatHaveFinished == assets.maxPrimaryThreads + assets.numPrimaryJobExeThreads; });

	sharedResources.nextPhaseJob = update1NextPhaseJob;
	renderPass.update2LastThread(this, sharedResources, sharedResources.renderPass);
	sharedResources.currentWorkStealingQueues = &sharedResources.workStealingQueues[0u];
	sharedResources.update(this);
	sharedResources.numThreadsThatHaveFinished = 0u;
	++(sharedResources.generation);
	lock.unlock();
	sharedResources.conditionVariable.notify_all();

	currentWorkStealingDeque = &workStealDeques[0u];
	gpuCompletionEventManager.update(this, sharedResources);
	streamingManager.update(this, sharedResources);
}