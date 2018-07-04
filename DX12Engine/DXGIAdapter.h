#pragma once
#include <DXGI1_5.h>
#include "IDXGIAdapterNotFoundException.h"

class DXGIAdapter
{
	IDXGIAdapter3* data;
public:
	DXGIAdapter(IDXGIFactory5* factory, HWND window, D3D_FEATURE_LEVEL featureLevel) : data(nullptr)
	{
		IDXGIAdapter1* adapter;
		DXGI_ADAPTER_DESC1 desc;

		for (auto adapterIndex = 0u; factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) // we'll start looking for directx 12 compatible graphics devices starting at index 0
		{
			adapter->GetDesc1(&desc);
			auto result = adapter->QueryInterface(IID_PPV_ARGS(&data));
			adapter->Release();
			if (result == S_OK)
			{
				if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && SUCCEEDED(D3D12CreateDevice(data, featureLevel, _uuidof(ID3D12Device), nullptr))) //this adapter is good
				{
					return;
				}
				data->Release();
			}
		}
		MessageBoxW(window, L"No DirectX 12 (D3D_FEATURE_LEVEL_11_0 or later) GPU found.", L"Error", MB_OK);
		throw IDXGIAdapterNotFoundException();
	}

	DXGIAdapter(DXGIAdapter&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	DXGIAdapter(const DXGIAdapter& other) = delete;

	void operator=(DXGIAdapter&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const DXGIAdapter& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator IDXGIAdapter3*()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	IDXGIAdapter3* operator->()
	{
		return data;
	}

	~DXGIAdapter()
	{
		if (data) data->Release();
	}
};