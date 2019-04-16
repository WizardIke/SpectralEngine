#pragma once
#include "UnorderedMultiProducerSingleConsumerQueue.h"
#include "SinglyLinked.h"
#include "LinkedTask.h"
#include <cstddef>
#include <cassert>

class PrimaryTaskFromOtherThreadQueue
{
public:
	using Task = LinkedTask;

	class StopRequest
	{
		friend class PrimaryTaskFromOtherThreadQueue;
		StopRequest** stopRequest;
		void(*callback)(StopRequest& stopRequest, void* tr, void* gr);
	public:
		StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr, void* gr)) : callback(callback1) {}
	};
private:
	UnorderedMultiProducerSingleConsumerQueue taskQueue;
	StopRequest* mStopRequest;
	const std::size_t queueIndex;

	template<class ThreadResources, class GlobalResources>
	void run(ThreadResources& threadResources, GlobalResources&)
	{
		threadResources.taskShedular.pushPrimaryTask(queueIndex, {this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			PrimaryTaskFromOtherThreadQueue& queue = *static_cast<PrimaryTaskFromOtherThreadQueue*>(requester);
			if(queue.mStopRequest != nullptr)
			{
				queue.mStopRequest->callback(*queue.mStopRequest, &threadResources, &globalResources);
				queue.mStopRequest = nullptr;
				return;
			}
			SinglyLinked* tasks = queue.taskQueue.popAll();
			while(tasks != nullptr)
			{
				Task& task = *static_cast<Task*>(tasks);
				tasks = tasks->next;
				task.execute(task, &threadResources, &globalResources);
			}
			queue.run(threadResources, globalResources);
		}});
	}
public:
	constexpr PrimaryTaskFromOtherThreadQueue(std::size_t queueIndex1) noexcept : queueIndex(queueIndex1), mStopRequest(nullptr) {}

	/*
	Can be called from any thread
	*/
	void push(Task& task) noexcept
	{
		taskQueue.push(&task);
	}

	/*
	Must be called from primary thread
	*/
	template<class ThreadResources, class GlobalResources>
	void start(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		assert(mStopRequest == nullptr && "Cannot start while not stopped");
		run(threadResources, globalResources);
	}

	/*
	Must be called from primary thread
	*/
	template<class ThreadResources, class GlobalResources>
	void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources&)
	{
		stopRequest.stopRequest = &mStopRequest;
		threadResources.taskShedular.pushPrimaryTask(queueIndex == 0u ? 1u : 0u, {&stopRequest, [](void* requester, ThreadResources&, GlobalResources&)
		{
			StopRequest& stopRequest = *static_cast<StopRequest*>(requester);
			*stopRequest.stopRequest = &stopRequest;
		}});
	}
};