#pragma once
#include "Windows.h"

class CriticalSection
{
public:
	CRITICAL_SECTION nativeCriticalSection;

	CriticalSection()
	{
		InitializeCriticalSection(&nativeCriticalSection);
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&nativeCriticalSection);
	}

	void lock()
	{
		EnterCriticalSection(&nativeCriticalSection);
	}

	void unlock()
	{
		LeaveCriticalSection(&nativeCriticalSection);
	}

	bool try_lock()
	{
		return TryEnterCriticalSection(&nativeCriticalSection) != FALSE;
	}
};