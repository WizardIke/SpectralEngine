#pragma once
#include <d3d12.h>
#include "ID3D12ResourceCreationFailedException.h"
#include <cstdint>

class D3D12Resource
{
	ID3D12Resource* data;
public:
	D3D12Resource(ID3D12Device* const device, const D3D12_HEAP_PROPERTIES& heapProperties, const D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_RESOURCE_STATES initialResourceState, const D3D12_CLEAR_VALUE* const optimizedClearValue) : data(nullptr)
	{
		auto hr = device->CreateCommittedResource(&heapProperties, heapFlags, &resourceDesc, initialResourceState, optimizedClearValue, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw ID3D12ResourceCreationFailedException();
	}

	D3D12Resource(ID3D12Device* const device, ID3D12Heap* const heap, const uint64_t heapOffset, const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_RESOURCE_STATES initialResourceState, const D3D12_CLEAR_VALUE* const optimizedClearValue) : data(nullptr)
	{
		HRESULT hr = device->CreatePlacedResource(heap, heapOffset, &resourceDesc, initialResourceState, optimizedClearValue, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw ID3D12ResourceCreationFailedException();
	}

	D3D12Resource(D3D12Resource&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12Resource() : data(nullptr) {}

	D3D12Resource(const D3D12Resource& other) = delete;

	void operator=(D3D12Resource&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12Resource& other) = delete;

	void operator=(nullptr_t) 
	{
		if (data) data->Release();
		data = nullptr;
	};

	ID3D12Resource*& set()
	{
		return data;
	}

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3D12Resource*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3D12Resource* operator->()
	{
		return data;
	}

	~D3D12Resource()
	{
		if (data) data->Release();
	}
};