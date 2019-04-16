#pragma once
#include <atomic>
#include "SinglyLinked.h"

#pragma warning(push)
#pragma warning(disable:4324) //warns about padding due to over alignment

class UnorderedMultiProducerSingleConsumerQueue
{
#if __cplusplus >= 201703L
	static constexpr std::size_t hardwareDestructiveInterferenceSize
		= std::hardware_destructive_interference_size;
#else
	static constexpr std::size_t hardwareDestructiveInterferenceSize = 64u;
#endif
	alignas(hardwareDestructiveInterferenceSize) std::atomic<SinglyLinked*> data = nullptr;
public:
	void push(SinglyLinked* value) noexcept
	{
		value->next = data.load(std::memory_order::memory_order_relaxed);
		while(!data.compare_exchange_weak(value->next, value, std::memory_order::memory_order_release, std::memory_order::memory_order_relaxed));
	}

	/*
	 * Returns nullptr if the queue is empty.
	 */
	SinglyLinked* popAll() noexcept
	{
		return data.exchange(nullptr, std::memory_order::memory_order_acquire);
	}
};

#pragma warning(pop)