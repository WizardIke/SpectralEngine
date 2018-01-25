#include "SharedResources.h"
#include "../WorkStealingQueue/WorkStealingQueue.h"
#include <Windowsx.h>
#include "BaseExecutor.h"

SharedResources::SharedResources(BaseExecutor* mainExecutor, bool fullScreen, bool vSync, bool enableGpuDebugging, unsigned int numThreads, WindowCallback windowCallback) :
	maxBackgroundThreads(unsigned int((float)numThreads / 4.0f + 0.5f) > 0u ? unsigned int((float)numThreads / 4.0f + 0.5f) : 1u),
	numPrimaryJobExeThreads(maxBackgroundThreads),
	maxPrimaryThreads(static_cast<int>(numThreads) - 1 - static_cast<int>(maxBackgroundThreads) < 0 ? 0u : (numThreads) - 1u - (maxBackgroundThreads)),
	maxThreads(numThreads > 1u ? numThreads : 2u),
	window(mainExecutor, windowCallback, [fullScreen]() {if (fullScreen) { return GetSystemMetrics(SM_CXVIRTUALSCREEN); } else return GetSystemMetrics(SM_CXSCREEN) / 2; }(),
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
	syncMutex(),
	conditionVariable(),
	timer(),
	streamingManager(graphicsEngine.graphicsDevice),
	soundEngine(),
	playerPosition(DirectX::XMFLOAT3(59.0f, 4.0f, 54.0f), DirectX::XMFLOAT3(0.0f, 0.2f, 0.0f)),
	inputManager(window, { PlayerPosition::mouseMoved, &playerPosition })
	
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

LRESULT SharedResources::windowCallbackImpl(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, SharedResources* sharedResources, BaseExecutor* executor)
{
	switch (message)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			sharedResources->inputManager.LeftPressed = true;
			return 0;
		case VK_RIGHT:
			sharedResources->inputManager.RightPressed = true;
			return 0;
		case VK_UP:
			sharedResources->inputManager.UpPressed = true;
			return 0;
		case VK_DOWN:
			sharedResources->inputManager.DownPressed = true;
			return 0;
		case 0x41: // A
			sharedResources->inputManager.APressed = true;
			return 0;
		case 0x44: // D
			sharedResources->inputManager.DPressed = true;
			return 0;
		case 0x53: // S
			sharedResources->inputManager.SPressed = true;
			return 0;
		case 0x57: // W
			sharedResources->inputManager.WPressed = true;
			return 0;
		case 0x58: // X
			sharedResources->inputManager.XPressed = true;
			return 0;
		case 0x5A: // Z
			sharedResources->inputManager.ZPressed = true;
			return 0;
		case VK_PRIOR: // Page up
			sharedResources->inputManager.PgUpPressed = true;
			return 0;
		case VK_NEXT: // Page Down
			sharedResources->inputManager.PgDownPressed = true;
			return 0;
		case VK_SPACE:
			sharedResources->inputManager.SpacePressed = true;
			return 0;
		case VK_ESCAPE:
			sharedResources->nextPhaseJob = BaseExecutor::quit;
			break;
		default:
			return 0;
		}
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_LEFT:
			sharedResources->inputManager.LeftPressed = false;
			return 0;
		case VK_RIGHT:
			sharedResources->inputManager.RightPressed = false;
			return 0;
		case VK_UP:
			sharedResources->inputManager.UpPressed = false;
			return 0;
		case VK_DOWN:
			sharedResources->inputManager.DownPressed = false;
			return 0;
		case 0x41: // A
			sharedResources->inputManager.APressed = false;
			return 0;
		case 0x44: // D
			sharedResources->inputManager.DPressed = false;
			return 0;
		case 0x53: // S
			sharedResources->inputManager.SPressed = false;
			return 0;
		case 0x57: // W
			sharedResources->inputManager.WPressed = false;
			return 0;
		case 0x58: // X
			sharedResources->inputManager.XPressed = false;
			return 0;
		case 0x5A: // Z
			sharedResources->inputManager.ZPressed = false;
			return 0;
		case VK_PRIOR: // Page up
			sharedResources->inputManager.PgUpPressed = false;
			return 0;
		case VK_NEXT: // Page Down
			sharedResources->inputManager.PgDownPressed = false;
			return 0;
		case VK_SPACE:
			sharedResources->inputManager.SpacePressed = false;
			return 0;
		default:
			return 0;
		}
	case WM_INPUT:
		sharedResources->inputManager.handleRawInput(lParam);
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