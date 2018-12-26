#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12Fence
{
	ID3D12Fence* data;
public:
	D3D12Fence(ID3D12Device* const device, const UINT64 initualValue, const D3D12_FENCE_FLAGS flags) : data(nullptr)
	{
		HRESULT hr = device->CreateFence(initualValue, flags, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}
	D3D12Fence(D3D12Fence&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12Fence() : data(nullptr) {}

	D3D12Fence(const D3D12Fence& other) = delete;

	void operator=(D3D12Fence&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12Fence& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3D12Fence*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3D12Fence* operator->()
	{
		return data;
	}

	~D3D12Fence()
	{
		if (data) data->Release();
	}
};