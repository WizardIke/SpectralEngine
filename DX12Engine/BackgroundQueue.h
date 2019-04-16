#pragma once
#include "SingleProducerSingleConsumerQueue.h"
#include <atomic>

template<class Task>
class BackgroundQueue : public SingleProducerSingleConsumerQueue<Task, 256u>
{
	std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
	bool try_lock() noexcept
	{
		return !locked.test_and_set(std::memory_order::memory_order_acquire);
	}

	void unlock() noexcept
	{
		locked.clear(std::memory_order::memory_order_release);
	}
};