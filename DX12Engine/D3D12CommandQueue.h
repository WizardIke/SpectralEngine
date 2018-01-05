#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12CommandQueue
{
	ID3D12CommandQueue* data;
public:
	D3D12CommandQueue(ID3D12Device* const device, const D3D12_COMMAND_QUEUE_DESC& desc) : data(nullptr)
	{
		HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}
	D3D12CommandQueue(D3D12CommandQueue&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12CommandQueue() : data(nullptr) {}

	D3D12CommandQueue(const D3D12CommandQueue& other) = delete;

	void operator=(D3D12CommandQueue&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12CommandQueue& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3D12CommandQueue*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3D12CommandQueue* operator->()
	{
		return data;
	}

	~D3D12CommandQueue()
	{
		if (data) data->Release();
	}
};