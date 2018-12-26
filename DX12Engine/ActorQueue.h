#pragma once
#include <atomic>
#include "SinglyLinked.h"

#pragma warning(push)
#pragma warning(disable:4324)

class ActorQueue
{
#if __cplusplus >= 201703L
	static constexpr std::size_t hardwareDestructiveInterferenceSize
		= std::hardware_destructive_interference_size;
#else
	static constexpr std::size_t hardwareDestructiveInterferenceSize = 64u;
#endif

	inline static SinglyLinked* stub() 
	{
		static SinglyLinked value;
		return &value;
	}

	alignas(hardwareDestructiveInterferenceSize) std::atomic<SinglyLinked*> data = stub();
	alignas(hardwareDestructiveInterferenceSize) SinglyLinked* dequeuedData = nullptr;
public:
	/*
	 * Returns true if the consumer is stopped. This can be used to resume the actor.
	 */
	bool push(SinglyLinked* value)
	{
		SinglyLinked* oldData = data.load(std::memory_order::memory_order_relaxed);
		while(true)
		{
			if(oldData == stub())
			{
				value->next = nullptr;
				if(data.compare_exchange_weak(oldData, value, std::memory_order::memory_order_release, std::memory_order::memory_order_relaxed))
				{
					return true;
				}
			}
			else
			{
				value->next = oldData;
				if(data.compare_exchange_weak(oldData, value, std::memory_order::memory_order_release, std::memory_order::memory_order_relaxed))
				{
					return false;
				}
			}
		};
	}

	/*
	 * Returns nullptr if the queue is empty.
	 */
	SinglyLinked* popAll()
	{
		return data.exchange(nullptr, std::memory_order::memory_order_acquire);
	}

	/*
	 * Marks the consumer as stopped. Returns false if the queue isn't empty.
	 */
	bool stop()
	{
		SinglyLinked* oldData = nullptr;
		return data.compare_exchange_strong(oldData, stub(), std::memory_order::memory_order_release, std::memory_order::memory_order_relaxed);
	}
};

#pragma warning(pop)