#pragma once
#include <DXGI1_5.h>

class DXGIAdapter
{
	IDXGIAdapter3* data;
public:
	DXGIAdapter(IDXGIAdapter3* adapter) : data(adapter) {}

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