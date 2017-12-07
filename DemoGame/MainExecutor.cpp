#include "MainExecutor.h"
#include "Assets.h"

MainExecutor::MainExecutor(SharedResources* const sharedResources) :
	Executor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
{
#ifdef _DEBUG
	this->type = "MainExecutor";
#endif // _DEBUG
}

void MainExecutor::update2(std::unique_lock<std::mutex>&& lock)
{
	Assets* assets = (Assets*)sharedResources;
	assets->conditionVariable.wait(lock, [assets = assets]() {return assets->numThreadsThatHaveFinished == assets->maxPrimaryThreads + assets->numPrimaryJobExeThreads; });

	assets->nextPhaseJob = update1NextPhaseJob;
	renderPass.update2LastThread(this, assets->renderPass, assets->numThreadsThatHaveFinished);
	assets->numThreadsThatHaveFinished = 0u;
	++(assets->generation);
	std::swap(assets->currentWorkStealingQueues, assets->nextWorkStealingQueues);
	assets->update(this);
	lock.unlock();
	assets->conditionVariable.notify_all();

	swapWorkStealingDeques();
	vRamFreeingManager.update(this);
	streamingManager.update(this);
}