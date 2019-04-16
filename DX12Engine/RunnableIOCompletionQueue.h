#pragma once
#include "IOCompletionQueue.h"

class RunnableIOCompletionQueue : public IOCompletionQueue
{
public:
	class StopRequest
	{
		friend class RunnableIOCompletionQueue;
		StopRequest** stopRequest;
		void(*callback)(StopRequest& stopRequest, void* tr, void* gr);
	public:
		StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr, void* gr)) : callback(callback1) {}
	};
private:
	StopRequest* mStopRequest = nullptr;
public:
	template<class ThreadResources, class GlobalResources>
	void start(ThreadResources& threadResources, GlobalResources&)
	{
		threadResources.taskShedular.pushPrimaryTask(0u, { this, [](void* requester, ThreadResources& threadResources, GlobalResources& sharedResources)
		{
			RunnableIOCompletionQueue& queue = *static_cast<RunnableIOCompletionQueue*>(requester);
			auto stopRequest = queue.mStopRequest;
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

	void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources&)
	{
		stopRequest.stopRequest = &mStopRequest;
		threadResources.taskShedular.pushPrimaryTask(1u, {&stopRequest, [](void* requester, ThreadResources&, GlobalResources&)
		{
			StopRequest& stopRequest = *static_cast<StopRequest*>(requester);
			*stopRequest.stopRequest = &stopRequest;
		}});
	}
};