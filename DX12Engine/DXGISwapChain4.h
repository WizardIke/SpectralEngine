#pragma once
#include <DXGI1_5.h>
#include "HresultException.h"

class DXGISwapChain4
{
	IDXGISwapChain4* data;
public:
	DXGISwapChain4(IDXGIFactory5* const factory, IUnknown *commandQueue, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, IDXGIOutput *pRestrictToOutput)
	{
		IDXGISwapChain1* tempSwapChain;
		HRESULT hr = factory->CreateSwapChainForHwnd(commandQueue, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, &tempSwapChain);
		if (FAILED(hr)) throw HresultException(hr);
		tempSwapChain->QueryInterface(IID_PPV_ARGS(&data));
		tempSwapChain->Release();
	}
	DXGISwapChain4(DXGISwapChain4&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	DXGISwapChain4() : data(nullptr) {}

	DXGISwapChain4(const DXGISwapChain4& other) = delete;

	void operator=(DXGISwapChain4&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const DXGISwapChain4& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator IDXGISwapChain4*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	IDXGISwapChain4* operator->()
	{
		return data;
	}

	~DXGISwapChain4()
	{
		if (data) data->Release();
	}
};