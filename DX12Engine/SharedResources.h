#pragma once
#include "D3D12GraphicsEngine.h"
#include "Window.h"
#include "SoundEngine.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include "Timer.h"
#include "InputManager.h"
#include "Queue.h"
#include <mutex>
#include "Job.h"
#include "PrimaryWorkStealingQueue.h"
#include "StreamingManager.h"
#include "IOCompletionQueue.h"
#include "ThreadBarrier.h"
#include "AsynchronousFileManager.h"
struct ID3D12CommandList;

class SharedResources
{
protected:
	constexpr static unsigned int backgroundQueueStartingLength = 120u;

	void checkForWindowsMessages();
	using WindowCallback = LRESULT (CALLBACK *)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	template<class InputHandler>
	static LRESULT windowCallbackImpl(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, SharedResources* sharedResources, BaseExecutor* executor, InputHandler& inputHandler)
	{
		switch (message)
		{
		case WM_INPUT:
			sharedResources->inputManager.handleRawInput(lParam, inputHandler, *sharedResources);
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xFFF0) == SC_MINIMIZE)
			{
				DefWindowProc(hwnd, message, wParam, lParam);
				while (true)
				{
					MSG msg;
					GetMessage(&msg, nullptr, 0, 0);
					hwnd = msg.hwnd;
					message = msg.message;
					wParam = msg.wParam;
					lParam = msg.lParam;
					if (message == WM_CLOSE || message == WM_DESTROY || (message == WM_SYSCOMMAND && ((wParam & 0xFFF0) == SC_MAXIMIZE)))
					{
						break;
					}
					DefWindowProc(hwnd, message, wParam, lParam);
				}
				return DefWindowProc(hwnd, message, wParam, lParam);
			}
		case 0x8000: //do a Job
			Job((void*)wParam, (void(*)(void* requester, BaseExecutor* executor, SharedResources& sharedResources))lParam)(executor, *sharedResources);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		return 0;
	}
public:
	template<class SharedResources_t>
	static LRESULT CALLBACK windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_NCCREATE)
		{
			CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT*>(lParam);
			auto state = create->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
		}
		auto sharedResources = reinterpret_cast<SharedResources_t*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return windowCallbackImpl(hwnd, message, wParam, lParam, sharedResources, &sharedResources->mainExecutor, sharedResources->inputHandler);
	}

	SharedResources(bool fullScreen, bool vSync, bool enableGpuDebugging, unsigned int numThreads, WindowCallback windowCallback);

	const unsigned int maxBackgroundThreads;
	const unsigned int maxPrimaryThreads;
	const unsigned int maxThreads;
	unsigned int numPrimaryJobExeThreads; //not thread safe

	ThreadBarrier threadBarrier;
	Window window;
	D3D12GraphicsEngine graphicsEngine;

	std::unique_ptr<BaseExecutor*[]> executors;
	std::unique_ptr<PrimaryWorkStealingQueue*[]> workStealingQueues;
	PrimaryWorkStealingQueue** currentWorkStealingQueues;
	Queue<Job> backgroundQueue; //thread safe

	void(*nextPhaseJob)(BaseExecutor* executor, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock);

	StreamingManager streamingManager; //thread safe
	IOCompletionQueue ioCompletionQueue; //thread safe
	AsynchronousFileManager asynchronousFileManager;
	TextureManager textureManager; //thread safe
	MeshManager meshManager; //thread safe
	SoundEngine soundEngine;
	Timer timer;
	InputManager inputManager;
};