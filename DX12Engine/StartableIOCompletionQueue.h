#pragma once
#include "IOCompletionQueue.h"

class StartableIOCompletionQueue : public IOCompletionQueue
{
public:
	template<class ThreadResources, class GlobalResources>
	void start(ThreadResources& threadResources, GlobalResources& sharedResources)
	{
		threadResources.taskShedular.update1NextQueue().push({ this, [](void* requester, ThreadResources& threadResources, GlobalResources& sharedResources)
		{
			StartableIOCompletionQueue& queue = *reinterpret_cast<StartableIOCompletionQueue*>(requester);
			IOCompletionPacket task;
			while (queue.pop(task))
			{
				task(&threadResources, &sharedResources);
			}
			queue.start(threadResources, sharedResources);
		} });
	}
};