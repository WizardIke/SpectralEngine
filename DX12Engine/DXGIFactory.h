#pragma once
#include <DXGI1_5.h>
#include "HresultException.h"
#include "D3D12Debug.h"

class DXGIFactory
{
	IDXGIFactory5* data;
public:
	DXGIFactory(bool enableGpuDebugging) : data(nullptr)
	{
#ifndef NDEBUG
		D3D12Debug debugController;
		debugController->EnableDebugLayer();
		if (enableGpuDebugging)
		{
			ID3D12Debug1* debug1Controller;
			debugController->QueryInterface<ID3D12Debug1>(&debug1Controller);
			debug1Controller->SetEnableGPUBasedValidation(true);
			debug1Controller->Release();
		}
#else
		if (enableGpuDebugging)
		{
			D3D12Debug debugController;
			debugController->EnableDebugLayer();
			ID3D12Debug1* debug1Controller;
			debugController->QueryInterface<ID3D12Debug1>(&debug1Controller);
			debug1Controller->SetEnableGPUBasedValidation(true);
			debug1Controller->Release();
		}
#endif
		HRESULT hr;
#ifndef NDEBUG
		hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&data));
#else
		if (enableGpuDebugging)
		{
			hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&data));
		}
		else
		{
			hr = CreateDXGIFactory2(0u, IID_PPV_ARGS(&data));
		}
#endif
		if (FAILED(hr)) throw HresultException(hr);
	}
	DXGIFactory(DXGIFactory&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	DXGIFactory(const DXGIFactory& other) = delete;

	void operator=(DXGIFactory&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const DXGIFactory& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator IDXGIFactory5*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	IDXGIFactory5* operator->()
	{
		return data;
	}

	~DXGIFactory()
	{
		if (data) data->Release();
	}
};