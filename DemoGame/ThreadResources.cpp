#include "ThreadResources.h"
#include "GlobalResources.h"

ThreadResources::ThreadResources(unsigned int index, GlobalResources& globalResources, void(*endUpdate2)(ThreadResources& threadResources, void* context)) :
	mEndUpdate2(endUpdate2),
	taskShedular(index, globalResources.taskShedular),
	randomNumberGenerator(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
	streamingManager(*globalResources.graphicsEngine.graphicsDevice),
	renderPass(globalResources.graphicsEngine) 
{}

void ThreadResources::startPrimary(GlobalResources& globalResources)
{
	taskShedular.startPrimary(globalResources.taskShedular, *this, &globalResources);
}

void ThreadResources::startBackground(GlobalResources& globalResources)
{
	taskShedular.startBackground(globalResources.taskShedular, *this, &globalResources);
}

bool ThreadResources::endUpdate1(ThreadResources& threadResources, void* context)
{
	GlobalResources& globalResources = *static_cast<GlobalResources*>(context);
	const unsigned int primaryIndex = threadResources.taskShedular.primaryIndex();
	if(primaryIndex == 0u)
	{
		globalResources.graphicsEngine.startFrame(&threadResources);
		globalResources.beforeRender();
	}

	threadResources.taskShedular.endUpdate1(globalResources.taskShedular, endUpdate2);

	threadResources.renderPass.update1After(globalResources.graphicsEngine, globalResources.renderPass, globalResources.rootSignatures.rootSignature, primaryIndex);

	return false;
}

bool ThreadResources::endUpdate2(ThreadResources& threadResources, void* context)
{
	threadResources.mEndUpdate2(threadResources, context);
	return false;
}

void ThreadResources::mainEndUpdate2(ThreadResources& threadResources, void* context)
{
	GlobalResources& globalResources = *static_cast<GlobalResources*>(context);

	unsigned int primaryThreadCount = globalResources.taskShedular.lockAndGetPrimaryThreadCount();
	threadResources.renderPass.update2(threadResources, globalResources.renderPass, primaryThreadCount, threadResources.taskShedular.primaryIndex());

	if(primaryThreadCount != 1u) globalResources.readyToPresentEvent.wait();
	
	threadResources.renderPass.present(primaryThreadCount, globalResources.graphicsEngine, globalResources.window, globalResources.renderPass);
	globalResources.update();

	threadResources.taskShedular.endUpdate2Primary(globalResources.taskShedular, endUpdate1);
	threadResources.streamingManager.update(globalResources.streamingManager, &threadResources);
}

void ThreadResources::primaryEndUpdate2(ThreadResources& threadResources, void* context)
{
	GlobalResources& globalResources = *static_cast<GlobalResources*>(context);
	unsigned int primaryThreadCount = globalResources.taskShedular.lockAndGetPrimaryThreadCount();
	threadResources.renderPass.update2(threadResources, globalResources.renderPass, primaryThreadCount, threadResources.taskShedular.primaryIndex());

	unsigned int presentIndex = globalResources.readyToPresentCount.fetch_add(1u, std::memory_order_acq_rel);
	if (presentIndex == primaryThreadCount - 2u)
	{
		globalResources.readyToPresentEvent.notify();
		globalResources.readyToPresentCount.store(0u, std::memory_order_relaxed);
	}

	threadResources.taskShedular.endUpdate2Primary(globalResources.taskShedular, endUpdate1);
	threadResources.streamingManager.update(globalResources.streamingManager, &threadResources);
}

static void backgroundPrepairForUpdate2(ThreadResources& threadResources, void* context)
{
	GlobalResources& globalResources = *static_cast<GlobalResources*>(context);
	threadResources.renderPass.update1After(globalResources.graphicsEngine, globalResources.renderPass, globalResources.rootSignatures.rootSignature, threadResources.taskShedular.primaryIndex());
}

void ThreadResources::backgroundEndUpdate2(ThreadResources& threadResources, void* context)
{
	GlobalResources& globalResources = *static_cast<GlobalResources*>(context);
	unsigned int primaryThreadCount = globalResources.taskShedular.lockAndGetPrimaryThreadCount();
	threadResources.renderPass.update2(threadResources, globalResources.renderPass, primaryThreadCount, threadResources.taskShedular.primaryIndex());

	unsigned int presentIndex = globalResources.readyToPresentCount.fetch_add(1u, std::memory_order_acq_rel);
	if (presentIndex == primaryThreadCount - 2u)
	{
		globalResources.readyToPresentEvent.notify();
		globalResources.readyToPresentCount.store(0u, std::memory_order_relaxed);
	}
	threadResources.taskShedular.endUpdate2Background<backgroundPrepairForUpdate2>(globalResources.taskShedular, endUpdate1, threadResources, context);
	threadResources.streamingManager.update(globalResources.streamingManager, &threadResources);
}