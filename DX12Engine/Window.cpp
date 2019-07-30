#include "Window.h"
#include "GraphicsEngine.h"
#ifndef NDEBUG
#include <string>
#endif

static constexpr DWORD makeBorderlessStyle(DWORD style)
{
	return style & ~((DWORD)WS_CAPTION | (DWORD)WS_MAXIMIZEBOX | (DWORD)WS_MINIMIZEBOX | (DWORD)WS_SYSMENU | (DWORD)WS_THICKFRAME);
}

Window::Window(void* callbackData, LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam),
	unsigned int fullscreenWidth, unsigned int fullscreenHeight, int fullscreenPositionX, int fullscreenPositionY,
	unsigned int windowedWidth, unsigned int windowedHeight, unsigned int windowedPositionX, unsigned int windowedPositionY,
	State state, bool vSync) :
	state(state),
	windowedState(state == State::fullscreen ? State::normal : state),
	vSync(vSync),
	mWidth(state == State::fullscreen ? fullscreenWidth : windowedWidth),
	mHeight(state == State::fullscreen ? fullscreenHeight : windowedHeight),
	mPositionX(state == State::fullscreen ? fullscreenPositionX : windowedPositionX),
	mPositionY(state == State::fullscreen ? fullscreenPositionY : windowedPositionY),
	windowedWidth(windowedWidth),
	windowedHeight(windowedHeight),
	windowedPositionX(windowedPositionX),
	windowedPositionY(windowedPositionY),
	fullscreenWidth(fullscreenWidth),
	fullscreenHeight(fullscreenHeight),
	fullscreenPositionX(fullscreenPositionX),
	fullscreenPositionY(fullscreenPositionY)
{
	auto instanceHandle = GetModuleHandleW(nullptr);

	WNDCLASSEX windowClassDesc;
	windowClassDesc.style = CS_HREDRAW | CS_VREDRAW;
	windowClassDesc.lpfnWndProc = WndProc;
	windowClassDesc.cbClsExtra = 0;
	windowClassDesc.cbWndExtra = 0;
	windowClassDesc.hInstance = instanceHandle;
	windowClassDesc.hIcon = LoadIconW(nullptr, IDI_WINLOGO);
	windowClassDesc.hIconSm = windowClassDesc.hIcon;
	windowClassDesc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	windowClassDesc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	windowClassDesc.lpszMenuName = nullptr;
	windowClassDesc.lpszClassName = applicationName;
	windowClassDesc.cbSize = sizeof(WNDCLASSEX);

	RegisterClassExW(&windowClassDesc);

	// Create the window with the screen settings and get the handle to it.
	style = WS_OVERLAPPEDWINDOW;
	auto currentStyle = state == State::fullscreen ? makeBorderlessStyle(style) : style;
	auto extendedStyle = WS_EX_APPWINDOW;
#ifdef NDEBUG //topmost windows cover the window used for debugging
	if (state == State::fullscreen)
	{
		extendedStyle |= WS_EX_TOPMOST;
	}
#endif
	windowHandle = CreateWindowExW(extendedStyle, applicationName, applicationName, currentStyle, mPositionX, mPositionY, mWidth, mHeight, nullptr, nullptr, instanceHandle, callbackData);
}

void Window::onResize(unsigned int width, unsigned int height)
{
	mWidth = width;
	mHeight = height;

	for (unsigned int i = 0u; i != frameBufferCount; ++i)
	{
		buffers[i] = nullptr;
	}
	swapChain->ResizeBuffers(0u, width, height, DXGI_FORMAT_UNKNOWN, 0u);

	for (unsigned int i = 0u; i != frameBufferCount; ++i)
	{
		HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i].set()));
		assert(hr == S_OK);

#ifndef NDEBUG
		std::wstring name = L"backbuffer render target ";
		name += std::to_wstring(i);
		buffers[i]->SetName(name.c_str());
#endif
	}
}

void Window::onMove(int posX, int posY)
{
	mPositionX = posX;
	mPositionY = posY;
}

void Window::onStateChanged(State newState)
{
	if (state != State::fullscreen)
	{
		state = newState;
	}
}

void Window::setForgroundAndShow()
{
	if (state == State::normal)
	{
		ShowWindow(windowHandle, SW_SHOWNORMAL);
	}
	else
	{
		ShowWindow(windowHandle, SW_SHOWMAXIMIZED);
	}
	
	SetForegroundWindow(windowHandle);
	SetFocus(windowHandle);
	ShowCursor(FALSE);
}

Window::~Window() 
{
	if (windowHandle != nullptr)
	{
		shutdown();
	}
}

void Window::destroy()
{
	shutdown();
	windowHandle = nullptr;
}

void Window::shutdown()
{
	ShowCursor(TRUE);
	DestroyWindow(windowHandle);
	UnregisterClassW(applicationName, nullptr);
}

void Window::createSwapChain(GraphicsEngine& graphicsEngine, IDXGIFactory5* dxgiFactory)
{
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
	fullscreenDesc.RefreshRate.Numerator = 0u;
	fullscreenDesc.RefreshRate.Denominator = 1u;
	fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	fullscreenDesc.Windowed = TRUE;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.BufferCount = frameBufferCount;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = 0u;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1u;
	swapChainDesc.SampleDesc.Quality = 0u;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	swapChain = DXGISwapChain4(dxgiFactory, graphicsEngine.directCommandQueue, windowHandle, &swapChainDesc, &fullscreenDesc, nullptr);

	for (unsigned int i = 0u; i != frameBufferCount; ++i)
	{
		HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i].set()));
		assert(hr == S_OK);

#ifndef NDEBUG
		std::wstring name = L"backbuffer render target ";
		name += std::to_wstring(i);
		buffers[i]->SetName(name.c_str());
#endif
	}
}

uint32_t Window::getCurrentBackBufferIndex()
{
	return swapChain->GetCurrentBackBufferIndex();
}

ID3D12Resource* Window::getBuffer(uint32_t index)
{
	return buffers[index];
}

void Window::present()
{
	auto hr = swapChain->Present(vSync, 0u);
	if (FAILED(hr)) { throw HresultException(hr); }
}

void Window::setFullScreen()
{
	// Make the window borderless.
	SetWindowLong(windowHandle, GWL_STYLE, makeBorderlessStyle(style));

	windowedWidth = mWidth;
	windowedHeight = mHeight;
	windowedPositionX = mPositionX;
	windowedPositionY = mPositionY;
	windowedState = state;
	state = State::fullscreen;
	SetWindowPos(windowHandle,
#ifdef NDEBUG //topmost windows cover the window used for debugging
		HWND_TOPMOST,
#else
		HWND_NOTOPMOST,
#endif
		fullscreenPositionX, fullscreenPositionY, fullscreenWidth, fullscreenHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);
	ShowWindow(windowHandle, SW_SHOWMAXIMIZED);
}

void Window::setWindowed()
{
	SetWindowLong(windowHandle, GWL_STYLE, style);
	
	state = windowedState;
	SetWindowPos(windowHandle, HWND_NOTOPMOST, windowedPositionX, windowedPositionY, windowedWidth, windowedHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);
	if (windowedState == State::normal)
	{
		ShowWindow(windowHandle, SW_SHOWNORMAL);
	}
}

bool Window::processMessagesForAllWindowsCreatedOnCurrentThread()
{
	MSG message;
	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != FALSE)
	{
		if (message.message == WM_QUIT)
		{
			return true;
		}
		DispatchMessage(&message);
	}
	return false;
}