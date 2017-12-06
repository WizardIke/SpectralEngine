#pragma once
#include <atomic>

class Spinlock
{
	std::atomic_flag lockFlag = ATOMIC_FLAG_INIT;
public:
	void lock()
	{
		while (lockFlag.test_and_set(std::memory_order_acquire));  // spin until lock acquired
	}

	void unlock()
	{
		lockFlag.clear(std::memory_order_release);
	}

	bool try_lock()
	{
		return lockFlag.test_and_set(std::memory_order_acquire);
	}
};