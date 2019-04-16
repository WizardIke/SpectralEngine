#pragma once
#include "IOCompletionQueue.h"
#include <atomic>

class RunnableIOCompletionQueue : public IOCompletionQueue
{
public:
	class StopRequest
	{
	public:
		void(*callback)(StopRequest& stopRequest, void* tr, void* gr);
	};
private:
	std::atomic<StopRequest*> mStopRequest = nullptr;
public:
	template<class ThreadResources, class GlobalResources>
	void start(ThreadResources& threadResources, GlobalResources&)
	{
		threadResources.taskShedular.pushPrimaryTask(0u, { this, [](void* requester, ThreadResources& threadResources, GlobalResources& sharedResources)
		{
			RunnableIOCompletionQueue& queue = *static_cast<RunnableIOCompletionQueue*>(requester);
			auto stopRequest = queue.mStopRequest.load(std::memory_order_acquire);
			if(stopRequest != nullptr)
			{
				stopRequest->callback(*stopRequest, &threadResources, &sharedResources);
				return;
			}
			IOCompletionPacket task;
			while (queue.pop(task))
			{
				task(&threadResources, &sharedResources);
			}
			queue.start(threadResources, sharedResources);
		} });
	}

	void stop(StopRequest& stopRequest)
	{
		mStopRequest.store(&stopRequest, std::memory_order_release);
	}
};