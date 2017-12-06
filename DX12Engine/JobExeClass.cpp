#include "JobExeClass.h"
#include "HresultException.h"
#include "SharedResources.h"

JobExeClass::JobExeClass(SharedResources*const sharedResources) :
	JobExeClassPrivateData(sharedResources),
	thread() {}

void JobExeClass::run()
{
	thread = std::thread(JobExeClass::mRun, static_cast<BaseExecutor*const>(this));
}

void JobExeClass::runBackgroundJobs(Job job)
{
	bool found;
	do
	{
		job(this);
		found = sharedResources->backgroundQueue.pop(job);
	} while (found);
}

void JobExeClass::endFrame(std::unique_lock<std::mutex>&& lock)
{
	startEndFrame(std::move(lock));
	Job job;
	bool found = sharedResources->backgroundQueue.pop(job);
	if (found)
	{
		bool wasFirstWorkStealingDecks = sharedResources->isFirstWorkStealingDecks();
		{
			std::lock_guard<decltype(sharedResources->syncMutex)> lock(sharedResources->syncMutex);
			--(sharedResources->numPrimaryJobExeThreads);
			--(sharedResources->numThreadsThatHaveFinishedEndFrame); //adjust for number of currently active threads dropping so other threads don't leave early
		}

		HRESULT hr = opaqueDirectCommandAllocators[frameIndex]->Reset();
		if (FAILED(hr)) throw HresultException(hr);

		hr = transparentDirectCommandAllocators[frameIndex]->Reset();
		if (FAILED(hr)) throw HresultException(hr);

		runBackgroundJobs(job);
		getIntoCorrectStateAfterDoingBackgroundJob(wasFirstWorkStealingDecks);
	}
	else
	{
		BaseExecutor::beginUpdate();
	}
}

void JobExeClass::getIntoCorrectStateAfterDoingBackgroundJob(bool wasFirstWorkStealingDecks)
{
	std::unique_lock<std::mutex> lock(sharedResources->syncMutex);
	++(sharedResources->numPrimaryJobExeThreads);
	//haven't got past BeginScene
	if (sharedResources->nextPhaseJob == beginRenderingNextPhaseJob)
	{
		sharedResources->numThreadsThatHaveFinishedEndFrame += 1u;//incase threads are waiting on beginUpdate
		frameIndex = sharedResources->graphicsEngine.frameIndex;

		if (sharedResources->isFirstWorkStealingDecks() == wasFirstWorkStealingDecks)
		{
			std::swap(currentWorkStealingDeque, nextWorkStealingDeque);
		}
		lock.unlock();

		BaseExecutor::updateWithoutSwappingWorkStealingDeque();
	}
	else if (sharedResources->nextPhaseJob == beginSceneNextPhaseJob)
	{
		sharedResources->numThreadsThatHaveFinishedEndFrame += 1u;//incase threads are waiting on beginUpdate
		sharedResources->numThreadsThatHaveFinishedBeginRendering += 1u;
		frameIndex = sharedResources->graphicsEngine.frameIndex;

		if (sharedResources->isFirstWorkStealingDecks() == wasFirstWorkStealingDecks)
		{
			std::swap(currentWorkStealingDeque, nextWorkStealingDeque);
		}
		lock.unlock();

		BaseExecutor::updateWithoutSwappingWorkStealingDeque();
	}
	else
	{
		sharedResources->numThreadsThatHaveFinishedBeginScene += 1u;
		//threads have all finished beginUpdate
		frameIndex = sharedResources->graphicsEngine.frameIndex;

		if (sharedResources->isFirstWorkStealingDecks() == wasFirstWorkStealingDecks)
		{
			std::swap(currentWorkStealingDeque, nextWorkStealingDeque);
		}
		lock.unlock();

		opaqueDirectCommandList = commandLists[frameIndex].directCommandList3DBeginScene;
		HRESULT hr = opaqueDirectCommandList->Reset(opaqueDirectCommandAllocators[frameIndex], nullptr);
		if (FAILED(hr)) throw HresultException(hr);

		if (FAILED(hr)) throw HresultException(hr);
		transparentDirectCommandList = commandLists[frameIndex].transparentDirectCommandList3D;
		hr = transparentDirectCommandList->Reset(transparentDirectCommandAllocators[frameIndex], nullptr);
		if (FAILED(hr)) throw HresultException(hr);

		opaqueDirectCommandList->SetGraphicsRootSignature(sharedResources->rootSignatures.rootSignature);
		transparentDirectCommandList->SetGraphicsRootSignature(sharedResources->rootSignatures.rootSignature);
		ID3D12DescriptorHeap* temp = sharedResources->graphicsEngine.mainDescriptorHeap;
		opaqueDirectCommandList->SetDescriptorHeaps(1u, &temp);
		transparentDirectCommandList->SetDescriptorHeaps(1u, &temp);
		opaqueDirectCommandList->SetGraphicsRootDescriptorTable(4u, temp->GetGPUDescriptorHandleForHeapStart());
		transparentDirectCommandList->SetGraphicsRootDescriptorTable(4u, temp->GetGPUDescriptorHandleForHeapStart());
		D3D12_VIEWPORT viewPort;
		viewPort.Height = static_cast<FLOAT>(sharedResources->graphicsEngine.window.height);
		viewPort.Width = static_cast<FLOAT>(sharedResources->graphicsEngine.window.width);
		viewPort.MaxDepth = 1.0f;
		viewPort.MinDepth = 0.0f;
		viewPort.TopLeftX = 0.0f;
		viewPort.TopLeftY = 0.0f;
		opaqueDirectCommandList->RSSetViewports(1u, &viewPort);
		transparentDirectCommandList->RSSetViewports(1u, &viewPort);
		D3D12_RECT scissorRect;
		scissorRect.top = 0;
		scissorRect.left = 0;
		scissorRect.bottom = sharedResources->graphicsEngine.window.height;
		scissorRect.right = sharedResources->graphicsEngine.window.width;
		opaqueDirectCommandList->RSSetScissorRects(1u, &scissorRect);
		transparentDirectCommandList->RSSetScissorRects(1u, &scissorRect);

		D3D12_CPU_DESCRIPTOR_HANDLE backBufferRenderTargetViewHandle = sharedResources->graphicsEngine.renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		backBufferRenderTargetViewHandle.ptr += frameIndex * sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
		D3D12_CPU_DESCRIPTOR_HANDLE depthSencilViewHandle = sharedResources->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		opaqueDirectCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetViewHandle, TRUE, &depthSencilViewHandle);
		transparentDirectCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetViewHandle, TRUE, &depthSencilViewHandle);
		sharedResources->perFrameConstantBuffers.bind(opaqueDirectCommandList, frameIndex);
		sharedResources->perFrameConstantBuffers.bind(transparentDirectCommandList, frameIndex);

		copyIndex += 1u;
		if (copyIndex == Settings::frameBufferCount) { copyIndex = 0u; }
		copyCommandList = copyCommandLists[copyIndex];
		hr = copyCommandAllocators[copyIndex]->Reset();
		if (FAILED(hr)) { throw HresultException(hr); }
		hr = copyCommandList->Reset(copyCommandAllocators[copyIndex], nullptr);
		if (FAILED(hr)) throw HresultException(hr);

		vRamFreeingManager.update(this);
		ramToVramUploader.update(this);
	}
}