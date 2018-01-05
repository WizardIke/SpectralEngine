#pragma once
#include <d3d12.h>
#include "HresultException.h"
#include <cstdint>

class D3D12RootSignature
{
	ID3D12RootSignature* data;
public:
	D3D12RootSignature() : data(nullptr) {}

	D3D12RootSignature(D3D12RootSignature&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12RootSignature(ID3D12Device* graphicsDevice, uint32_t nodeMask, const void* serializedData, size_t serializedDataLength)
	{
		auto hr = graphicsDevice->CreateRootSignature(nodeMask, serializedData, serializedDataLength, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}

	D3D12RootSignature(const D3D12RootSignature& other) = delete;

	void operator=(D3D12RootSignature&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12RootSignature& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3D12RootSignature*&()
	{
		return data;
	}

	ID3D12RootSignature*& get()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3D12RootSignature* operator->()
	{
		return data;
	}

	~D3D12RootSignature()
	{
		if (data) data->Release();
	}
};