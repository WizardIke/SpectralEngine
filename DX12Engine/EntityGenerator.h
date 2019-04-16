#pragma once
#include <atomic>

class EntityGenerator
{
	std::atomic<unsigned long> nextValue;
public:
	unsigned long generate() noexcept
	{
		return nextValue.fetch_add(1u, std::memory_order_relaxed);
	}
};