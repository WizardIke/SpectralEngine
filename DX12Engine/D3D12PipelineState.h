#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12PipelineState
{
	ID3D12PipelineState* data;
public:
	D3D12PipelineState(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		auto hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}

	D3D12PipelineState(D3D12PipelineState&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3D12PipelineState() : data(nullptr) {}

	D3D12PipelineState(const D3D12PipelineState& other) = delete;

	void operator=(D3D12PipelineState&& other)
	{
		this->data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3D12PipelineState& other) = delete;

	operator ID3D12PipelineState*&()
	{
		return data;
	}

	ID3D12PipelineState*& get()
	{
		return data;
	}

	ID3D12PipelineState* operator->()
	{
		return data;
	}
	~D3D12PipelineState()
	{
		if (data) data->Release();
	}
};