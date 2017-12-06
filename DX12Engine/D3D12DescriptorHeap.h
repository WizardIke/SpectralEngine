#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12DescriptorHeap
{
	ID3D12DescriptorHeap* data;
public:
	D3D12DescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_DESC& desc) : data(nullptr)
	{
		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}
	D3D12DescriptorHeap(D3D12DescriptorHeap&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12DescriptorHeap() : data(nullptr) {}

	D3D12DescriptorHeap(const D3D12DescriptorHeap& other) = delete;

	void operator=(D3D12DescriptorHeap&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12DescriptorHeap& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3D12DescriptorHeap*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3D12DescriptorHeap* operator->()
	{
		return data;
	}

	~D3D12DescriptorHeap()
	{
		if (data) data->Release();
	}
};