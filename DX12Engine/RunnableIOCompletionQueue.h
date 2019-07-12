#pragma once
#include "IOCompletionQueue.h"

class RunnableIOCompletionQueue : public IOCompletionQueue
{
public:
	class StopRequest
	{
		friend class RunnableIOCompletionQueue;
		StopRequest** stopRequest;
		void(*callback)(StopRequest& stopRequest, void* tr);
	public:
		StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr)) : callback(callback1) {}
	};
private:
	StopRequest* mStopRequest = nullptr;
public:
	template<class ThreadResources>
	void start(ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushPrimaryTask(0u, { this, [](void* requester, ThreadResources& threadResources)
		{
			RunnableIOCompletionQueue& queue = *static_cast<RunnableIOCompletionQueue*>(requester);
			auto stopRequest = queue.mStopRequest;
			if(stopRequest != nullptr)
			{
				stopRequest->callback(*stopRequest, &threadResources);
				return;
			}
			IOCompletionPacket task;
			while (queue.pop(task))
			{
				task(&threadResources);
			}
			queue.start(threadResources);
		} });
	}

	void stop(StopRequest& stopRequest, ThreadResources& threadResources)
	{
		stopRequest.stopRequest = &mStopRequest;
		threadResources.taskShedular.pushPrimaryTask(1u, {&stopRequest, [](void* requester, ThreadResources&)
		{
			StopRequest& stopRequest = *static_cast<StopRequest*>(requester);
			*stopRequest.stopRequest = &stopRequest;
		}});
	}
};