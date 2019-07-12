#pragma once
#undef min
#undef max
#include <pcg_random.hpp>
#include <frameBufferCount.h>
#include <StreamingManager.h>
#include <TaskShedular.h>
#include "RenderPass1.h"
class GlobalResources;
#undef min
#undef max

class ThreadResources
{
	friend class GlobalResources;

	void (*mEndUpdate2)(ThreadResources& threadResources, void* context);

	static bool endUpdate1(ThreadResources& threadResources, void* context);
	static bool endUpdate2(ThreadResources& threadResources, void* context);
	static void mainEndUpdate2(ThreadResources& threadResources, void* context);
	static void primaryEndUpdate2(ThreadResources& threadResources, void* context);
	static void backgroundEndUpdate2(ThreadResources& threadResources, void* context);

	void startPrimary(GlobalResources& globalResources);
	void startBackground(GlobalResources& globalResources);
	ThreadResources(unsigned int index, GlobalResources& globalResources, void(*endUpdate2)(ThreadResources& threadResources, void* context));
public:
	TaskShedular<ThreadResources>::ThreadLocal taskShedular;
	pcg32 randomNumberGenerator;
	StreamingManager::ThreadLocal streamingManager;
	RenderPass1::Local renderPass;
};