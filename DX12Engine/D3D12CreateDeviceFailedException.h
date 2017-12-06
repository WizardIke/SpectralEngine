#pragma once
#include "HresultException.h"

class D3D12CreateDeviceFailedException : public HresultException
{
public:
	D3D12CreateDeviceFailedException(HRESULT hresult) : HresultException(hresult) {}
};