#pragma once
#include <atomic>
#include <limits>
#include <cstddef>
#ifdef max
#undef max
#endif
#include <array>

// Push = single producer
// Pop = single consumer (same thread as push)
// Steal = multiple consumers

// All methods, including Push, may fail. Re-issue the request
// if that occurs (spinwait).

template<class value_type, unsigned long capacity>
class WorkStealingStack {
private:
	template<class value_type>
	friend class WorkStealingStackReference;
	std::atomic<unsigned long> top; // queue
	std::atomic<unsigned long> bottom; // stack
									   // Circular array
	std::array<value_type, capacity> values;
public:
	WorkStealingStack() : top(1), bottom(1) {}

	WorkStealingStack(const WorkStealingStack&) = delete;
	WorkStealingStack(WorkStealingStack<value_type, capacity>&& other) : top(other.top.load()), bottom(other.bottom.load()), values(std::move(other.values)) {}

	~WorkStealingStack() {}

	// Single producer
	bool push(const value_type& item) noexcept 
	{
		auto oldtop = top.load(std::memory_order_relaxed);
		auto oldbottom = bottom.load(std::memory_order_relaxed); //this thread is the only thread that changes bottom
		auto numtasks = oldbottom - oldtop;

		if (numtasks > capacity) {
			return false; //might be full
		}
		values[oldbottom % capacity] = item;
		bottom.store(oldbottom + 1u, std::memory_order_release); //other threads need to see changes to values
		return true;
	}

	// Single consumer
	bool pop(value_type& result) noexcept 
	{
		auto oldBottom = bottom.load(std::memory_order_acquire);
		auto newbottom = oldBottom - 1u;
		bottom.exchange(newbottom, std::memory_order::memory_order_seq_cst);
		auto oldTop = top.load(std::memory_order_acquire);

		if (newbottom > oldTop)
		{
			result = values[newbottom % capacity];
			return true;
		}
		if (newbottom == oldTop)
		{
			result = values[newbottom % capacity];
			if (top.compare_exchange_strong(oldTop, oldTop + 1u, std::memory_order_release, std::memory_order_relaxed))
			{
				//we got the job
				bottom.store(oldBottom, std::memory_order_release);
				return true;
			}
			bottom.store(oldBottom, std::memory_order_release);
			return false;
		}
		bottom.store(oldBottom, std::memory_order_release);
		return false;
	}

	// Multiple consumer.
	bool steal(value_type& result) noexcept 
	{
		auto oldtop = top.load(std::memory_order_acquire);
		auto oldbottom = bottom.load(std::memory_order_acquire);

		if (oldbottom <= oldtop) return false;

		result = values[oldtop % capacity];
		// Make sure that we are not contending for the item.
		if (!top.compare_exchange_strong(oldtop, oldtop + 1u, std::memory_order_release, std::memory_order_relaxed)) {
			return false;
		}
		return true;
	}
};