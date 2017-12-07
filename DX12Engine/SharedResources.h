#pragma once
#include "D3D12GraphicsEngine.h"
#include "Window.h"
#include "SoundEngine.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include "Timer.h"
#include "InputManager.h"
#include "PlayerPosition.h"
#include "MainCamera.h"
#include "Frustum.h"
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

	static void checkForWindowsMessages(BaseExecutor* const executor);
public:
	SharedResources(BaseExecutor* mainExecutor, bool fullScreen, bool vSync);

	const unsigned int maxBackgroundThreads = unsigned int((float)std::thread::hardware_concurrency() / 4.0f + 0.5f) > 0u ?
		unsigned int((float)std::thread::hardware_concurrency() / 4.0f + 0.5f) : 1u;

	const unsigned int maxPrimaryThreads = static_cast<int>(std::thread::hardware_concurrency()) - 1 - static_cast<int>(maxBackgroundThreads) < 0 ?
		0u : (std::thread::hardware_concurrency()) - 1u - (maxBackgroundThreads);

	const unsigned int maxThreads = std::thread::hardware_concurrency() > 1u ? std::thread::hardware_concurrency() : 2u;

	std::mutex syncMutex; //thread safe
	std::condition_variable conditionVariable; //thread safe
	unsigned int numPrimaryJobExeThreads; //not thread safe
	Window window;
	D3D12GraphicsEngine graphicsEngine;

	std::unique_ptr<WorkStealingStackReference<Job>[]> workStealingQueues;
	unsigned int numThreadsThatHaveFinished = 0u; //not thread safe
	unsigned int generation = 0u; //not thread safe

	Queue<Job> backgroundQueue; //thread safe
	WorkStealingStackReference<Job>* currentWorkStealingQueues;
	WorkStealingStackReference<Job>* nextWorkStealingQueues;

	void(*nextPhaseJob)(BaseExecutor* executor, std::unique_lock<std::mutex>&& lock);

	StreamingManager streamingManager;
	TextureManager textureManager; //thread safe
	MeshManager meshManager; //thread safe
	SoundEngine soundEngine;
	Timer timer;
	Frustum mainFrustum;
	InputManager inputManager;
	PlayerPosition playerPosition;

	bool isFirstWorkStealingDecks()
	{
		return currentWorkStealingQueues == workStealingQueues.get();
	}
};