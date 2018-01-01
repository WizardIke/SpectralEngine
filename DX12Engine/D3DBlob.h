#pragma once
#include <d3d12.h>

class D3DBlob
{
	ID3DBlob* data;
public:
	D3DBlob() : data(nullptr) {}

	D3DBlob(D3DBlob&& other) : data(other.data)
	{
		other.data = nullptr;
	}

	D3DBlob(const D3DBlob& other) = delete;

	void operator=(D3DBlob&& other)
	{
		if (data) data->Release();
		data = other.data;
		other.data = nullptr;
	}

	void operator=(const D3DBlob& other) = delete;

	void operator=(nullptr_t)
	{
		if (data) data->Release();
		data = nullptr;
	};

	bool operator==(std::nullptr_t)
	{
		return data == nullptr;
	}

	operator ID3DBlob*&()
	{
		return data;
	}

	ID3DBlob*& get()
	{
		return data;
	}

	operator void* ()
	{
		return data;
	}

	ID3DBlob* operator->()
	{
		return data;
	}

	~D3DBlob()
	{
		if (data) data->Release();
	}
};