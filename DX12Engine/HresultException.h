#pragma once
#include <Windows.h>
#undef max
#include "Exception.h"

class HresultException : public Exception
{
public:
	const HRESULT hrsult;
	HresultException(const HRESULT hrsult) : hrsult(hrsult) {}
};