#include "ThreadResources.h"
#include "GlobalResources.h"

ThreadResources::ThreadResources(unsigned int index, GlobalResources& globalResources, void(*endUpdate2)(ThreadResources& threadResources, GlobalResources& globalResources)) :
	mEndUpdate2(endUpdate2),
	taskShedular(index, globalResources.taskShedular), 
	gpuCompletionEventManager(),
	randomNumberGenerator(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
	streamingManager(globalResources.graphicsEngine.graphicsDevice, globalResources.streamingManager, index),
	renderPass(globalResources.graphicsEngine) 
{
}

void ThreadResources::start(GlobalResources& globalResources)
{
	taskShedular.sync(globalResources.taskShedular);
	taskShedular.start(globalResources.taskShedular, *this, globalResources);
}

bool ThreadResources::initialize1(ThreadResources& threadResources, GlobalResources& globalResources)
{
	threadResources.taskShedular.runBackgroundTasks(globalResources.taskShedular, threadResources, globalResources);
	threadResources.streamingManager.update(globalResources.streamingManager);

	std::unique_lock<std::mutex> lock(globalResources.taskShedular.barrier().mutex());
	if (globalResources.taskShedular.barrier().waiting_count() == globalResources.taskShedular.primaryThreadCount() - 1u)
	{
		globalResources.streamingManager.update<ThreadResources, GlobalResources>(threadResources.taskShedular);
		
		globalResources.taskShedular.barrier().waiting_count() = 0u;
		++(globalResources.taskShedular.barrier().generation());
		lock.unlock();
		globalResources.taskShedular.barrier().notify_all();
	}
	else
	{
		++(globalResources.taskShedular.barrier().waiting_count());
		globalResources.taskShedular.barrier().wait(lock, [&generation = globalResources.taskShedular.barrier().generation(), gen = globalResources.taskShedular.barrier().generation()]() {return gen != generation; });
		lock.unlock();
	}

	return false;
}

bool ThreadResources::initialize2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	std::unique_lock<std::mutex> lock(globalResources.taskShedular.barrier().mutex());
	if (globalResources.taskShedular.barrier().waiting_count() == globalResources.taskShedular.primaryThreadCount() - 1u)
	{
		globalResources.taskShedular.setNextPhaseTask(initialize3);

		globalResources.taskShedular.barrier().waiting_count() = 0u;
		++(globalResources.taskShedular.barrier().generation());
		lock.unlock();
		globalResources.taskShedular.barrier().notify_all();
	}
	else
	{
		++(globalResources.taskShedular.barrier().waiting_count());
		globalResources.taskShedular.barrier().wait(lock, [&generation = globalResources.taskShedular.barrier().generation(), gen = globalResources.taskShedular.barrier().generation()]() {return gen != generation; });
		lock.unlock();
	}

	return false;
}

bool ThreadResources::initialize3(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	threadResources.taskShedular.endUpdate2Primary(globalResources.taskShedular, endUpdate1, primaryThreadCount, updateIndex);
	return false;
}

bool ThreadResources::endUpdate1(ThreadResources& threadResources, GlobalResources& globalResources)
{
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex();
	const bool isFirstThread = updateIndex == 0u;
	threadResources.renderPass.update1(threadResources, globalResources.graphicsEngine, globalResources.renderPass, isFirstThread);

	threadResources.taskShedular.endUpdate1(globalResources.taskShedular, endUpdate2, updateIndex);

	threadResources.renderPass.update1After(globalResources.graphicsEngine, globalResources.renderPass, globalResources.rootSignatures.rootSignature, isFirstThread);

	return false;
}

bool ThreadResources::endUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	threadResources.mEndUpdate2(threadResources, globalResources);
	return false;
}

bool ThreadResources::quit(ThreadResources& threadResources, GlobalResources& globalResources)
{
	return true;
}

void ThreadResources::mainEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	threadResources.renderPass.update2(threadResources, globalResources, globalResources.renderPass, primaryThreadCount);

	if(primaryThreadCount != 1u) globalResources.readyToPresentEvent.wait();
	
	threadResources.renderPass.present(primaryThreadCount, globalResources.graphicsEngine, globalResources.window, globalResources.renderPass);
	globalResources.update(threadResources);

	threadResources.taskShedular.endUpdate2Primary(globalResources.taskShedular, endUpdate1, primaryThreadCount, updateIndex);
	
	threadResources.gpuCompletionEventManager.update(globalResources.graphicsEngine.frameIndex, &threadResources, &globalResources);
	threadResources.streamingManager.update(globalResources.streamingManager);
}

void ThreadResources::primaryEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	threadResources.renderPass.update2(threadResources, globalResources, globalResources.renderPass, primaryThreadCount);

	unsigned int presentIndex = globalResources.readyToPresentCount.fetch_add(1u, std::memory_order::memory_order_acq_rel);
	if (presentIndex == primaryThreadCount - 2u)
	{
		globalResources.readyToPresentEvent.notify();
		globalResources.readyToPresentCount.store(0u, std::memory_order::memory_order_relaxed);
	}

	threadResources.taskShedular.endUpdate2Primary(globalResources.taskShedular, endUpdate1, primaryThreadCount, updateIndex);

	threadResources.gpuCompletionEventManager.update(globalResources.graphicsEngine.frameIndex, &threadResources, &globalResources);
	threadResources.streamingManager.update(globalResources.streamingManager);
}

static void backgroundEndUpdate(ThreadResources& threadResources, GlobalResources& globalResources)
{
	threadResources.gpuCompletionEventManager.update(globalResources.graphicsEngine.frameIndex, &threadResources, &globalResources);
	threadResources.streamingManager.update(globalResources.streamingManager);
}

static void backgroundPrepairForUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	threadResources.renderPass.update1After(globalResources.graphicsEngine, globalResources.renderPass, globalResources.rootSignatures.rootSignature, false);
}

void ThreadResources::backgroundEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	threadResources.renderPass.update2(threadResources, globalResources, globalResources.renderPass, primaryThreadCount);

	unsigned int presentIndex = globalResources.readyToPresentCount.fetch_add(1u, std::memory_order::memory_order_acq_rel);
	if (presentIndex == primaryThreadCount - 2u)
	{
		globalResources.readyToPresentEvent.notify();
		globalResources.readyToPresentCount.store(0u, std::memory_order::memory_order_relaxed);
	}
	threadResources.taskShedular.endUpdate2Background<backgroundEndUpdate, backgroundPrepairForUpdate2>(globalResources.taskShedular, threadResources, globalResources, endUpdate1, primaryThreadCount, updateIndex);
}