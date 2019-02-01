#pragma once
#include <d3d12.h>
#include "D3D12CreateDeviceFailedException.h"
#include "DXGIAdapter.h"

class D3D12Device
{
	ID3D12Device* data;
public:
	D3D12Device(IDXGIAdapter3* adapter, const D3D_FEATURE_LEVEL featureLevel) : data(nullptr)
	{
		HRESULT hr = D3D12CreateDevice(adapter, featureLevel, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw D3D12CreateDeviceFailedException(hr);
	}

	D3D12Device(ID3D12Device* device) : data(device) {}

	D3D12Device(D3D12Device&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12Device() : data(nullptr) {}

	D3D12Device(const D3D12Device& other) = delete;

	void operator=(D3D12Device&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12Device& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3D12Device*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3D12Device* operator->()
	{
		return data;
	}

	~D3D12Device()
	{
		if (data) data->Release();
	}
};