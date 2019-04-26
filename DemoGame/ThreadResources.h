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

	void (*mEndUpdate2)(ThreadResources& threadResources, GlobalResources& globalResources);

	/*
	Uses all the threads to process backgroundTasks.
	*/
	static bool initialize1(ThreadResources& threadResources, GlobalResources& globalResources);
	/*
	After setting this state, threads might disagree about weather the state is initialize1 or initialize2 for one frame.
	However, this is harmless and they will all agree that the state has transitioned to state initialize3 at the same time.
	*/
	static bool initialize2(ThreadResources& threadResources, GlobalResources& globalResources);
	/*
	Finishes initializing.
	*/
	static bool initialize3(ThreadResources& threadResources, GlobalResources& globalResources);

	static bool endUpdate1(ThreadResources& threadResources, GlobalResources& globalResources);
	static bool endUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);
	static void mainEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);
	static void primaryEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);
	static void backgroundEndUpdate2(ThreadResources& threadResources, GlobalResources& globalResources);

	void start(GlobalResources& globalResources);
	ThreadResources(unsigned int index, GlobalResources& globalResources, void(*endUpdate2)(ThreadResources& threadResources, GlobalResources& globalResources));
public:
	TaskShedular<ThreadResources, GlobalResources>::ThreadLocal taskShedular;
	pcg32 randomNumberGenerator;
	StreamingManager::ThreadLocal streamingManager;
	RenderPass1::Local renderPass;
};