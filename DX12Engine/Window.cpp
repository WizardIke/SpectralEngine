#include "Window.h"
#include "D3D12GraphicsEngine.h"

Window::Window(void* callbackData, LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam),
	unsigned int width, unsigned int height, int positionX, int positionY, bool fullScreen, bool vSync) : fullScreen(fullScreen), vSync(vSync)
{
	auto instanceHandle = GetModuleHandleW(nullptr);

	WNDCLASSEX windowClassDesc;
	windowClassDesc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
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
	windowHandle = CreateWindowExW(WS_EX_APPWINDOW, applicationName, applicationName, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, positionX, positionY, width, height, nullptr, nullptr, instanceHandle, callbackData);

	mwidth = width;
	mheight = height;
}

void Window::resize(unsigned int width, unsigned int height, int positionX, int positionY)
{
	mwidth = width;
	mheight = height;

	MoveWindow(windowHandle, positionX, positionY, width, height, FALSE);

	swapChain->ResizeBuffers(0u, 0u, 0u, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, 0u);
}

void Window::setForgroundAndShow()
{
	ShowWindow(windowHandle, SW_SHOW);
	SetForegroundWindow(windowHandle);
	SetFocus(windowHandle);
	ShowCursor(FALSE);
}

Window::~Window() 
{
	ShowCursor(TRUE);
	if (fullScreen)
	{
		ChangeDisplaySettingsW(nullptr, 0u);
	}
	DestroyWindow(windowHandle);
	UnregisterClassW(applicationName, nullptr);
}

void Window::createSwapChain(D3D12GraphicsEngine& graphicsEngine, IDXGIFactory5* dxgiFactory)
{
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDescPointer = nullptr;
	if (fullScreen)
	{
		fullscreenDesc.RefreshRate.Numerator = 0u;
		fullscreenDesc.RefreshRate.Denominator = 1u;
		fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		fullscreenDesc.Windowed = FALSE;
		fullscreenDescPointer = &fullscreenDesc;
	}
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = mwidth;
	swapChainDesc.Height = mheight;
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

	swapChain = DXGISwapChain4(dxgiFactory, graphicsEngine.directCommandQueue, windowHandle, &swapChainDesc, fullscreenDescPointer, nullptr);


#ifndef NDEBUG
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		std::wstring name = L"backbuffer render target ";
		name += std::to_wstring(i);
		getBuffer(i)->SetName(name.c_str());
	}
#endif
}

uint32_t Window::getCurrentBackBufferIndex()
{
	return swapChain->GetCurrentBackBufferIndex();
}

ID3D12Resource* Window::getBuffer(uint32_t index)
{
	ID3D12Resource* buffer;
	HRESULT hr = swapChain->GetBuffer(index, IID_PPV_ARGS(&buffer));
	assert(hr == S_OK);
	return buffer;
}

void Window::present()
{
	auto hr = swapChain->Present(vSync, 0u);
	if (FAILED(hr)) { throw HresultException(hr); }
}

void Window::setFullScreen()
{
	ChangeDisplaySettingsW(nullptr, CDS_FULLSCREEN);
	swapChain->SetFullscreenState(TRUE, nullptr);
}

void Window::setWindowed()
{
	swapChain->SetFullscreenState(FALSE, nullptr);
	ChangeDisplaySettingsW(nullptr, 0u);
}

bool Window::processMessagesForAllWindowsOnCurrentThread()
{
	MSG message;
	BOOL error;
	while (true)
	{
		error = (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE));
		if (error == 0) { break; } //no messages
		else if (error < 0) //error reading message
		{
			return false;
		}
		DispatchMessage(&message);
	}
	return true;
}