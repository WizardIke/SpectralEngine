#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12GraphicsCommandListPointer
{
	ID3D12GraphicsCommandList* data;
public:
	D3D12GraphicsCommandListPointer(ID3D12Device* const device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pCommandAllocator, ID3D12PipelineState* pInitialState)
	{
		HRESULT hr = device->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}

	D3D12GraphicsCommandListPointer(D3D12GraphicsCommandListPointer&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12GraphicsCommandListPointer() : data(nullptr) {}

	D3D12GraphicsCommandListPointer(const D3D12GraphicsCommandListPointer& other) = delete;

	void operator=(D3D12GraphicsCommandListPointer&& other)
	{
		this->data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12GraphicsCommandListPointer& other) = delete;

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
	~D3D12GraphicsCommandListPointer()
	{
		if (data) data->Release();
	}
};