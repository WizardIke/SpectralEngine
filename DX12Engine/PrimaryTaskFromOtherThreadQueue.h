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
		void(*callback)(StopRequest& stopRequest, void* tr);
	public:
		StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr)) : callback(callback1) {}
	};
private:
	UnorderedMultiProducerSingleConsumerQueue taskQueue;
	StopRequest* mStopRequest;
	const std::size_t queueIndex;

	template<class ThreadResources>
	void run(ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushPrimaryTask(queueIndex, {this, [](void* requester, ThreadResources& threadResources)
		{
			PrimaryTaskFromOtherThreadQueue& queue = *static_cast<PrimaryTaskFromOtherThreadQueue*>(requester);
			if(queue.mStopRequest != nullptr)
			{
				queue.mStopRequest->callback(*queue.mStopRequest, &threadResources);
				queue.mStopRequest = nullptr;
				return;
			}
			SinglyLinked* tasks = queue.taskQueue.popAll();
			while(tasks != nullptr)
			{
				Task& task = *static_cast<Task*>(tasks);
				tasks = tasks->next;
				task.execute(task, &threadResources);
			}
			queue.run(threadResources);
		}});
	}
public:
	constexpr PrimaryTaskFromOtherThreadQueue(std::size_t queueIndex1) noexcept : queueIndex(queueIndex1), mStopRequest(nullptr) {}

#if defined(_MSC_VER)
	/*
	Initialization of array members doesn't seam to have copy elision in some cases when it should in c++17.
	*/
	PrimaryTaskFromOtherThreadQueue(PrimaryTaskFromOtherThreadQueue&&);
#endif

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
	template<class ThreadResources>
	void start(ThreadResources& threadResources)
	{
		assert(mStopRequest == nullptr && "Cannot start while not stopped");
		run(threadResources);
	}

	/*
	Must be called from primary thread
	*/
	template<class ThreadResources>
	void stop(StopRequest& stopRequest, ThreadResources& threadResources)
	{
		stopRequest.stopRequest = &mStopRequest;
		threadResources.taskShedular.pushPrimaryTask(queueIndex == 0u ? 1u : 0u, {&stopRequest, [](void* requester, ThreadResources&)
		{
			StopRequest& stopRequest = *static_cast<StopRequest*>(requester);
			*stopRequest.stopRequest = &stopRequest;
		}});
	}
};