#include "MainThread.h"
#include "SharedResources.h"

MainThread::MainThread(SharedResources* const sharedResources) :
	BaseExecutor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
{
#ifdef _DEBUG
	this->type = "MainThread";
#endif // _DEBUG
}

void MainThread::endFrame(std::unique_lock<std::mutex>&& lock)
{
	HRESULT hr;

	while (sharedResources->numThreadsThatHaveFinishedEndFrame != sharedResources->settings.maxPrimaryThreads + sharedResources->numPrimaryJobExeThreads)
	{
		sharedResources->conditionVariable.wait(lock);
	}

	sharedResources->nextPhaseJob = beginRenderingNextPhaseJob;
	sharedResources->numThreadsThatHaveFinishedBeginScene = 0u; //reset for beginScene
	std::swap(sharedResources->currentWorkStealingQueues, sharedResources->nextWorkStealingQueues);

	sharedResources->graphicsEngine.endScene(transparentDirectCommandList);
	hr = opaqueDirectCommandList->Close();
	if (FAILED(hr)) throw HresultException(hr);
	hr = transparentDirectCommandList->Close();
	if (FAILED(hr)) throw HresultException(hr);
	hr = copyCommandList->Close();
	if (FAILED(hr)) { throw HresultException(hr); }
	sharedResources->copyCommandLists[sharedResources->numCopyCommandLists] = copyCommandList;
	sharedResources->graphicsEngine.copyCommandQueue->ExecuteCommandLists(sharedResources->numCopyCommandLists + 1u, sharedResources->copyCommandLists.get());
	sharedResources->numCopyCommandLists = 0u;

	sharedResources->commandLists[sharedResources->numCommandLists] = opaqueDirectCommandList;
	sharedResources->commandLists[sharedResources->numCommandLists * 2u + 1] = transparentDirectCommandList;
	sharedResources->graphicsEngine.endFrame(sharedResources->commandLists.get(), (sharedResources->numCommandLists + 1u) * 2u);
	sharedResources->numCommandLists = 0u;

	frameIndex = sharedResources->graphicsEngine.frameIndex;
	sharedResources->graphicsEngine.waitForPreviousFrame();
	sharedResources->update(this);
	++(sharedResources->numThreadsThatHaveFinishedEndFrame);
	lock.unlock();
	sharedResources->conditionVariable.notify_all();

	beginUpdate();
}