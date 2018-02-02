#include "SharedResources.h"
#include "../WorkStealingQueue/WorkStealingQueue.h"
#include <Windowsx.h>
#include "BaseExecutor.h"

SharedResources::SharedResources(bool fullScreen, bool vSync, bool enableGpuDebugging, unsigned int numThreads, WindowCallback windowCallback) :
	maxBackgroundThreads(unsigned int((float)numThreads / 4.0f + 0.5f) > 0u ? unsigned int((float)numThreads / 4.0f + 0.5f) : 1u),
	numPrimaryJobExeThreads(maxBackgroundThreads),
	maxPrimaryThreads(static_cast<int>(numThreads) - 1 - static_cast<int>(maxBackgroundThreads) < 0 ? 0u : (numThreads) - 1u - (maxBackgroundThreads)),
	maxThreads(numThreads > 1u ? numThreads : 2u),
	window(this, windowCallback, [fullScreen]() {if (fullScreen) { return GetSystemMetrics(SM_CXVIRTUALSCREEN); } else return GetSystemMetrics(SM_CXSCREEN) / 2; }(),
		[fullScreen]() {if (fullScreen) { return GetSystemMetrics(SM_CYVIRTUALSCREEN); } else return GetSystemMetrics(SM_CYSCREEN) / 2; }(),
		[fullScreen]() {if (fullScreen) { return 0; } else return GetSystemMetrics(SM_CXSCREEN) / 5; }(),
		[fullScreen]() {if (fullScreen) { return 0; } else return GetSystemMetrics(SM_CYSCREEN) / 5; }(), fullScreen, vSync),
	graphicsEngine(window, fullScreen, vSync, enableGpuDebugging),
	textureManager(),
	backgroundQueue(backgroundQueueStartingLength),
	workStealingQueues(new WorkStealingStackReference<Job>[(maxThreads) * 2u]),
	currentWorkStealingQueues(&workStealingQueues[maxThreads]),
	nextPhaseJob([](BaseExecutor* executor, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock)
	{
		lock.unlock();
	}),
	timer(),
	streamingManager(graphicsEngine.graphicsDevice),
	soundEngine(),
	inputManager()
{}
	
void SharedResources::checkForWindowsMessages()
{
	MSG message;
	BOOL error;
	while (true)
	{
		error = (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE));
		if (error == 0) { break; } //no messages
		else if (error < 0) //error reading message
		{
			nextPhaseJob = BaseExecutor::quit;
		}
		DispatchMessage(&message);
	}
}