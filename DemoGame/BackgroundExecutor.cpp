#include "BackgroundExecutor.h"
#include "Assets.h"
#include <D3D12GraphicsEngine.h>
/*
#include <Windows.h>
#include <iostream>
#include <sstream>

#define DBOUT( s )            \
{                             \
	std::wostringstream os_;    \
	os_ << s;                   \
	OutputDebugStringW( os_.str().c_str() );  \
}
*/

BackgroundExecutor::BackgroundExecutor(SharedResources& sharedResources) : Executor(sharedResources)
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

	++(sharedResources.threadBarrier.waiting_count());

	if (sharedResources.threadBarrier.waiting_count() == sharedResources.maxPrimaryThreads + sharedResources.numPrimaryJobExeThreads)
	{
		sharedResources.threadBarrier.notify_all();
	}

	sharedResources.threadBarrier.wait(lock, [&generation = sharedResources.threadBarrier.generation(), gen = sharedResources.threadBarrier.generation()]() {return gen != generation; });

	//DBOUT(L"thread 1: " << sharedResources.mainCamera.position().x << L"\n");

	Job job;
	IOCompletionPacket ioJob;
	bool firstJobType = true;
	bool found = sharedResources.backgroundQueue.pop(job);
	if (!found) {
		found = sharedResources.ioCompletionQueue.removeIOCompletionPacket(ioJob);
		firstJobType = false;
	}
	if (found)
	{
		--(sharedResources.numPrimaryJobExeThreads);
		lock.unlock();

		gpuCompletionEventManager.update(this, sharedResources);
		streamingManager.update(this, sharedResources);

		if (firstJobType)
		{
			job(this, sharedResources);
		}
		else
		{
			ioJob(this, sharedResources);
		}
		Executor::runBackgroundJobs(sharedResources);
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
	std::unique_lock<std::mutex> lock(sharedResources.threadBarrier.mutex());

	if (assets->nextPhaseJob == update2NextPhaseJob)
	{
		if (assets->threadBarrier.waiting_count() != 0u)
		{
			assets->threadBarrier.wait(lock, [oldGen = assets->threadBarrier.generation(), &gen = assets->threadBarrier.generation()]() {return oldGen != gen; });

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