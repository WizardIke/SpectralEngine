#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12CommandQueuePointer
{
	ID3D12CommandQueue* data;
public:
	D3D12CommandQueuePointer(ID3D12Device* const device, const D3D12_COMMAND_QUEUE_DESC& desc) : data(nullptr)
	{
		HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}
	D3D12CommandQueuePointer(D3D12CommandQueuePointer&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12CommandQueuePointer() : data(nullptr) {}

	D3D12CommandQueuePointer(const D3D12CommandQueuePointer& other) = delete;

	void operator=(D3D12CommandQueuePointer&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12CommandQueuePointer& other) = delete;

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

	~D3D12CommandQueuePointer()
	{
		if (data) data->Release();
	}
};