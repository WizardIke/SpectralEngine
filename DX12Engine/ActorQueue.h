#pragma once
#include <atomic>
#include "SinglyLinked.h"

class ActorQueue
{
#if __cplusplus >= 201703L
	static constexpr std::size_t hardwareDestructiveInterferenceSize
		= std::hardware_destructive_interference_size;
#else
	static constexpr std::size_t hardwareDestructiveInterferenceSize = 64u;
#endif

	static SinglyLinked stub;

	alignas(hardwareDestructiveInterferenceSize) std::atomic<SinglyLinked*> data = &stub;
	alignas(hardwareDestructiveInterferenceSize) SinglyLinked* dequeuedData = nullptr;
public:
	/*
	 * Returns true if the consumer is stopped. This can be used to resume the actor.
	 */
	bool push(SinglyLinked* value)
	{
		SinglyLinked* oldData = data.load(std::memory_order::memory_order_relaxed);
		do
		{
			value->next = oldData;
		} while(!data.compare_exchange_weak(oldData, value, std::memory_order::memory_order_release, std::memory_order::memory_order_relaxed));
		return oldData == &stub;
	}

	/*
	 * Returns false if the queue is empty.
	 */
	bool pop(SinglyLinked*& value)
	{
		SinglyLinked* list = dequeuedData;
		if(!list)
		{
			list = data.exchange(nullptr, std::memory_order::memory_order_acquire);
			if(list == nullptr)
			{
				return false;
			}
		}
		
		value = list;
		dequeuedData = list->next;
		return true;
	}

	/*
	 * Marks the consumer as stopped. Returns false if the queue isn't empty.
	 */
	bool stop()
	{
		SinglyLinked* oldData = nullptr;
		return data.compare_exchange_strong(oldData, &stub, std::memory_order::memory_order_release, std::memory_order::memory_order_relaxed);
	}
};