#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12CommandAllocator
{
	ID3D12CommandAllocator* data;
public:
	D3D12CommandAllocator(ID3D12Device* const device, D3D12_COMMAND_LIST_TYPE type)
	{
		HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}

	D3D12CommandAllocator(D3D12CommandAllocator&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12CommandAllocator() : data(nullptr) {}

	D3D12CommandAllocator(const D3D12CommandAllocator& other) = delete;

	void operator=(D3D12CommandAllocator&& other)
	{
		this->data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12CommandAllocator& other) = delete;

	operator ID3D12CommandAllocator*()
	{
		return data;
	}

	ID3D12CommandAllocator* operator->()
	{
		return data;
	}
	ID3D12CommandAllocator*& get()
	{
		return data;
	}

	~D3D12CommandAllocator()
	{
		if(data) data->Release();
	}
};