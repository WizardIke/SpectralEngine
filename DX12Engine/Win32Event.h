#pragma once
#include <windows.h>
#include "EventCreationException.h"

class Event
{
	HANDLE data;
public:
	Event(LPSECURITY_ATTRIBUTES securityAttributes, BOOL manualReset, BOOL initialState, LPCWCHAR name)
	{
		data = CreateEventW(securityAttributes, manualReset, initialState, name);
		if (!data) throw EventCreationException();
	}

	operator HANDLE() { return data; }

	~Event()
	{
		CloseHandle(data);
	}
};