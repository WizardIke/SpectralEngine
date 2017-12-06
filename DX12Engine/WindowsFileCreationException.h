#pragma once
#include "HresultException.h"

class WindowsFileCreationException : public HresultException
{
public:
	WindowsFileCreationException(HRESULT hr) : HresultException(hr) {}
};