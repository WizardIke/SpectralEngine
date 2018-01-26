#pragma once
#include "D3D12GraphicsEngine.h"
#include "Window.h"
#include "SoundEngine.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include "Timer.h"
#include "InputManager.h"
#include "PlayerPosition.h"
#include "Queue.h"
#include <mutex>
#include "Job.h"
#include "../WorkStealingQueue/WorkStealingQueue.h"
#include "StreamingManager.h"
struct ID3D12CommandList;

class SharedResources
{
protected:
	constexpr static unsigned int backgroundQueueStartingLength = 120u;

	void checkForWindowsMessages();
	using WindowCallback = LRESULT (CALLBACK *)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	static LRESULT windowCallbackImpl(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, SharedResources* sharedResources, BaseExecutor* executor);
public:
	template<BaseExecutor*(*getMainExecutor)(SharedResources& sharedResources)>
	static LRESULT CALLBACK windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_NCCREATE)
		{
			CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT*>(lParam);
			auto state = create->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
		}
		auto sharedResources = reinterpret_cast<SharedResources*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return windowCallbackImpl(hwnd, message, wParam, lParam, sharedResources, getMainExecutor(*sharedResources));
	}

	SharedResources(bool fullScreen, bool vSync, bool enableGpuDebugging, unsigned int numThreads, WindowCallback windowCallback);

	const unsigned int maxBackgroundThreads;
	const unsigned int maxPrimaryThreads;
	const unsigned int maxThreads;
	unsigned int numPrimaryJobExeThreads; //not thread safe

	std::mutex syncMutex; //thread safe
	std::condition_variable conditionVariable; //thread safe
	unsigned int numThreadsThatHaveFinished = 0u; //not thread safe
	unsigned int generation = 0u; //not thread safe

	Window window;
	D3D12GraphicsEngine graphicsEngine;

	std::unique_ptr<WorkStealingStackReference<Job>[]> workStealingQueues;
	Queue<Job> backgroundQueue; //thread safe
	WorkStealingStackReference<Job>* currentWorkStealingQueues;

	void(*nextPhaseJob)(BaseExecutor* executor, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock);

	StreamingManager streamingManager;
	TextureManager textureManager; //thread safe
	MeshManager meshManager; //thread safe
	SoundEngine soundEngine;
	Timer timer;
	PlayerPosition playerPosition;
	InputManager inputManager;

	bool isUpdate1()
	{
		return currentWorkStealingQueues == workStealingQueues.get();
	}
};