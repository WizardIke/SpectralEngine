#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12FencePointer
{
	ID3D12Fence* data;
public:
	D3D12FencePointer(ID3D12Device* const device, const UINT64 initualValue, const D3D12_FENCE_FLAGS flags) : data(nullptr)
	{
		HRESULT hr = device->CreateFence(initualValue, flags, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}
	D3D12FencePointer(D3D12FencePointer&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12FencePointer() : data(nullptr) {}

	D3D12FencePointer(const D3D12FencePointer& other) = delete;

	void operator=(D3D12FencePointer&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12FencePointer& other) = delete;

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

	~D3D12FencePointer()
	{
		if (data) data->Release();
	}
};