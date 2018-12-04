#pragma once
#undef min
#undef max
#include <pcg_random.hpp>
#include <GpuCompletionEventManager.h>
#include <frameBufferCount.h>
#include <StreamingManager.h>
#include <TaskShedular.h>
#include "RenderPass1.h"
class GlobalResources;
#undef min
#undef max

class ThreadResources
{
	void (*mEndUpdate2)(ThreadResources& threadResources, GlobalResources& globalResources);
public:
	TaskShedular<ThreadResources, GlobalResources>::ThreadLocal taskShedular;
	GpuCompletionEventManager<frameBufferCount> gpuCompletionEventManager;
	pcg32 randomNumberGenerator;
	StreamingManager::ThreadLocal streamingManager;
	RenderPass1::Local renderPass;

	void start(GlobalResources& globalResources);
	ThreadResources(unsigned int index, GlobalResources& globalResources, void(*endUpdate2)(ThreadResources& threadResources, GlobalResources& globalResources));

	static bool initialize1(ThreadResources& threadResources, GlobalResources& globalResources);
	static bool initialize2(ThreadResources& threadResources, GlobalResources& globalResources);
	static bool initialize3(ThreadResources& threadResources, GlobalResources& globalResources);
	static bool quit(ThreadResources& threadResources, GlobalResources& globalResources);
	static bool endUpdate1(ThreadResources& threadResources, GlobalResources& globalResources);
	static bool endUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);

	static void mainEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);
	static void primaryEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);
	static void backgroundEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);
};