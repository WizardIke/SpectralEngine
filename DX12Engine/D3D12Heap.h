#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12Heap
{
	ID3D12Heap* data;
public:
	D3D12Heap(ID3D12Device* const device, const D3D12_HEAP_DESC& desc) : data(nullptr)
	{
		HRESULT hr = device->CreateHeap(&desc, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}
	D3D12Heap(D3D12Heap&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12Heap() : data(nullptr) {}

	D3D12Heap(const D3D12Heap& other) = delete;

	void operator=(D3D12Heap&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12Heap& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3D12Heap*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3D12Heap* operator->()
	{
		return data;
	}

	~D3D12Heap()
	{
		if (data) data->Release();
	}
};