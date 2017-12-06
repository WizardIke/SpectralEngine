#pragma once
#include <Windows.h>
class BaseZoneClass;
class BaseJobExeClass;

class UnloadRequest
{
	BaseZoneClass* baseZoneClass;
	void(*unload)(void* const worker, BaseJobExeClass* const executor);
	UINT64 fenceValue;
	UINT frameIndex;
	bool primary;
public:
	UnloadRequest() {}
	UnloadRequest(bool primary, BaseZoneClass* const baseZoneClass, void(*unload)(void* const worker, BaseJobExeClass* const executor));

	bool update(BaseJobExeClass* const executor);
};