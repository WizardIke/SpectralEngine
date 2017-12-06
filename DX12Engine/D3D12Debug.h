#pragma once
#include <d3d12.h>
#include "HresultException.h"

class D3D12Debug
{
	ID3D12Debug* data;
public:
	D3D12Debug()
	{
		HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&data));
		if (FAILED(hr)) throw HresultException(hr);
	}

	ID3D12Debug* operator->()
	{
		return data;
	}

	operator ID3D12Debug*()
	{
		return data;
	}

	~D3D12Debug()
	{
		data->Release();
	}
};