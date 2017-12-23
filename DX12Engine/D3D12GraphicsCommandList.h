#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12GraphicsCommandList
{
	ID3D12GraphicsCommandList* data;
public:
	D3D12GraphicsCommandList(ID3D12Device* const device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pCommandAllocator, ID3D12PipelineState* pInitialState)
	{
		HRESULT hr = device->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}

	D3D12GraphicsCommandList(D3D12GraphicsCommandList&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12GraphicsCommandList() : data(nullptr) {}

	D3D12GraphicsCommandList(const D3D12GraphicsCommandList& other) = delete;

	void operator=(D3D12GraphicsCommandList&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	operator ID3D12GraphicsCommandList*()
	{
		return data;
	}

	ID3D12GraphicsCommandList*& get()
	{
		return data;
	}

	ID3D12GraphicsCommandList* operator->()
	{
		return data;
	}
	~D3D12GraphicsCommandList()
	{
		if (data) data->Release();
	}
};