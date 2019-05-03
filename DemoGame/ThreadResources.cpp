#include "ThreadResources.h"
#include "GlobalResources.h"

ThreadResources::ThreadResources(unsigned int index, GlobalResources& globalResources, void(*endUpdate2)(ThreadResources& threadResources, GlobalResources& globalResources)) :
	mEndUpdate2(endUpdate2),
	taskShedular(index, globalResources.taskShedular),
	randomNumberGenerator(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
	streamingManager(globalResources.graphicsEngine.graphicsDevice),
	renderPass(globalResources.graphicsEngine) 
{
}

void ThreadResources::start(GlobalResources& globalResources)
{
	taskShedular.sync(globalResources.taskShedular);
	taskShedular.start(globalResources.taskShedular, *this, globalResources);
}

bool ThreadResources::endUpdate1(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	const bool isFirstThread = updateIndex == 0u;
	threadResources.renderPass.update1(threadResources, globalResources, globalResources.renderPass, isFirstThread);
	if(isFirstThread)
	{
		globalResources.beforeRender();
	}

	threadResources.taskShedular.endUpdate1(globalResources.taskShedular, endUpdate2, updateIndex, primaryThreadCount);

	threadResources.renderPass.update1After(globalResources.graphicsEngine, globalResources.renderPass, globalResources.rootSignatures.rootSignature, isFirstThread);

	return false;
}

bool ThreadResources::endUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	threadResources.mEndUpdate2(threadResources, globalResources);
	return false;
}

void ThreadResources::mainEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	threadResources.taskShedular.beforeEndUpdate2(globalResources.taskShedular, endUpdate1, primaryThreadCount, updateIndex);
	threadResources.renderPass.update2(threadResources, globalResources, globalResources.renderPass, primaryThreadCount);

	if(primaryThreadCount != 1u) globalResources.readyToPresentEvent.wait();
	
	threadResources.renderPass.present(primaryThreadCount, globalResources.graphicsEngine, globalResources.window, globalResources.renderPass);
	globalResources.update();

	threadResources.taskShedular.endUpdate2Primary(globalResources.taskShedular, primaryThreadCount);
	threadResources.streamingManager.update(globalResources.streamingManager, &threadResources, &globalResources);
}

void ThreadResources::primaryEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	threadResources.taskShedular.beforeEndUpdate2(globalResources.taskShedular, endUpdate1, primaryThreadCount, updateIndex);
	threadResources.renderPass.update2(threadResources, globalResources, globalResources.renderPass, primaryThreadCount);

	unsigned int presentIndex = globalResources.readyToPresentCount.fetch_add(1u, std::memory_order_acq_rel);
	if (presentIndex == primaryThreadCount - 2u)
	{
		globalResources.readyToPresentEvent.notify();
		globalResources.readyToPresentCount.store(0u, std::memory_order_relaxed);
	}

	threadResources.taskShedular.endUpdate2Primary(globalResources.taskShedular, primaryThreadCount);
	threadResources.streamingManager.update(globalResources.streamingManager, &threadResources, &globalResources);
}

static void backgroundPrepairForUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	threadResources.renderPass.update1After(globalResources.graphicsEngine, globalResources.renderPass, globalResources.rootSignatures.rootSignature, false);
}

void ThreadResources::backgroundEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources)
{
	unsigned int primaryThreadCount;
	const unsigned int updateIndex = globalResources.taskShedular.incrementUpdateIndex(primaryThreadCount);
	threadResources.taskShedular.beforeEndUpdate2(globalResources.taskShedular, endUpdate1, primaryThreadCount, updateIndex);
	threadResources.renderPass.update2(threadResources, globalResources, globalResources.renderPass, primaryThreadCount);

	unsigned int presentIndex = globalResources.readyToPresentCount.fetch_add(1u, std::memory_order_acq_rel);
	if (presentIndex == primaryThreadCount - 2u)
	{
		globalResources.readyToPresentEvent.notify();
		globalResources.readyToPresentCount.store(0u, std::memory_order_relaxed);
	}
	threadResources.taskShedular.endUpdate2Background<backgroundPrepairForUpdate2>(globalResources.taskShedular, threadResources, globalResources, primaryThreadCount);
	threadResources.streamingManager.update(globalResources.streamingManager, &threadResources, &globalResources);
}