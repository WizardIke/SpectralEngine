#pragma once
#include <windows.h>
#undef min
#undef max
#include "EventCreationException.h"
#include <cassert>

class Event
{
	HANDLE data;
public:
	Event(LPSECURITY_ATTRIBUTES securityAttributes, BOOL manualReset, BOOL initialState, LPCWCHAR name)
	{
		data = CreateEventW(securityAttributes, manualReset, initialState, name);
		if (!data) throw EventCreationException();
	}

	void notify()
	{
		BOOL succeeded = SetEvent(data);
		assert(succeeded == TRUE);
	}

	void wait()
	{
		WaitForSingleObject(data, INFINITE);
	}

	void wait_for(unsigned long timeOut)
	{
		WaitForSingleObject(data, timeOut);
	}

	operator HANDLE() { return data; }

	~Event()
	{
		CloseHandle(data);
	}
};