#pragma once
#include <vector>
#include "frameBufferCount.h"
class BaseExecutor;

class VRamFreeingManager
{
public:
	class Request
	{
	public:
		void* requester;
		void(*unload)(void* const requester, BaseExecutor* const executor);
	};
private:
	std::vector<Request> requests[frameBufferCount];
public:
	void update(BaseExecutor* const executor);
	void addRequest(void* requester, void(*unloadCallback)(void* const requester, BaseExecutor* const executor), uint32_t frameIndex);
};