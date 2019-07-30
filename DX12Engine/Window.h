#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max
#undef WIN32_LEAN_AND_MEAN
#include "DXGISwapChain4.h"
#include <cstdint>
#include "D3D12Resource.h"
#include "frameBufferCount.h"
class GraphicsEngine;

class Window
{
	friend GraphicsEngine;
	static constexpr auto applicationName = L"Sprectral Engine";
	constexpr static float fieldOfView = 3.141592654f / 4.0f;
public:
	enum class State
	{
		fullscreen,
		maximized,
		normal
	};
private:
	HWND windowHandle;
	DWORD style;
	DXGISwapChain4 swapChain;
	D3D12Resource buffers[frameBufferCount];
	unsigned int fullscreenWidth, fullscreenHeight, fullscreenPositionX, fullscreenPositionY;
	unsigned int windowedWidth, windowedHeight, windowedPositionX, windowedPositionY;
	unsigned int mWidth, mHeight, mPositionX, mPositionY;
	State windowedState;
	State state;
	bool vSync;

	void createSwapChain(GraphicsEngine& graphicsEngine, IDXGIFactory5* dxgiFactory);

	void shutdown();
public:
	Window(void* callbackData, LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam),
		unsigned int fullscreenWidth, unsigned int fullscreenHeight, int fullscreenPositionX, int fullscreenPositionY,
		unsigned int windowedWidth, unsigned int windowedHeight, unsigned int windowedPositionX, unsigned int windowedPositionY,
		State state, bool vSync);
	~Window();

	void onMove(int posX, int posY);
	void onResize(unsigned int width, unsigned int height);
	void onStateChanged(State state);

	void setForgroundAndShow();

	unsigned int width() const
	{
		return mWidth;
	}
	unsigned int height() const
	{
		return mHeight;
	}

	bool isFullscreen() const
	{
		return state == State::fullscreen;
	}

	HWND native_handle()
	{
		return windowHandle;
	}

	uint32_t getCurrentBackBufferIndex();
	ID3D12Resource* getBuffer(uint32_t index);
	void present();
	void setFullScreen();
	void setWindowed();
	void setVSync(bool value) { vSync = value; }
	bool getVSync() { return vSync; }

	void destroy();

	void showCursor();
	void hideCursor();

	/*
	Returns true if the program should quit
	*/
	static bool processMessagesForAllWindowsCreatedOnCurrentThread();
};