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
	bool push(const value_type& item) noexcept {
		auto oldtop = top.load(std::memory_order_relaxed);
		auto oldbottom = bottom.load(std::memory_order_relaxed); //this thread is the only thread that changes bottom
		auto numtasks = oldbottom - oldtop;

		if (numtasks > capacity) {
			return false;
		}
		if (oldbottom == std::numeric_limits<unsigned long>::max()) { //bottum is going to overfrow
			auto newTop = top % capacity;
			auto newBottum = oldbottom - top + newTop;
			values[newBottum % capacity] = item;
			bottom.store(newBottum + 1u, std::memory_order_release);
			top.store(newTop, std::memory_order_release);
		}
		else {
			values[oldbottom % capacity] = item;
			bottom.store(oldbottom + 1u, std::memory_order_release); //other threads need to see changes to values
		}
		return true;
	}

	// Single consumer
	bool pop(value_type& result) noexcept {
		auto oldBottom = bottom.fetch_sub(1u, std::memory_order_acquire);
		auto oldTop = top.load(std::memory_order_acquire);

		auto newbottom = oldBottom - 1u;
		if (newbottom > oldTop)
		{
			result = values[newbottom % capacity];
			return true;
		}
		else if (newbottom == oldTop) //there was one element
		{
			if (top.compare_exchange_strong(oldTop, oldTop + 1u, std::memory_order_release, std::memory_order_relaxed))
			{
				//we got the job
				bottom.store(oldBottom, std::memory_order_release);
				result = values[newbottom % capacity];
				return true;
			}
			else//we were too late
			{
				bottom.store(oldBottom, std::memory_order_release);
				return false;
			}
		}

		bottom.store(oldBottom, std::memory_order_release);//was already empty
		return false;
	}

	// Multiple consumer.
	bool steal(value_type& result) noexcept {
		auto oldtop = top.load(std::memory_order_acquire);
		auto oldbottom = bottom.load(std::memory_order_acquire);

		if (oldbottom <= oldtop) return false;

		// Make sure that we are not contending for the item.
		if (!top.compare_exchange_strong(oldtop, oldtop + 1u, std::memory_order_release, std::memory_order_relaxed)) {
			return false;
		}

		result = values[oldtop % capacity];
		return true;
	}
};

template<class value_type>
class WorkStealingStackReference
{
private:
	//typedef int value_type;
	unsigned char* workStealingStack;
	// Circular array
	unsigned long capacity;
	typedef WorkStealingStack<value_type, 1024> WorkStealingStack_t;
	std::atomic<unsigned long>& top()
	{
		return *reinterpret_cast<std::atomic<unsigned long>* const>(workStealingStack + offsetof(WorkStealingStack_t, top));
	}
	std::atomic<unsigned long>& bottom()
	{
		return *reinterpret_cast<std::atomic<unsigned long>* const>(workStealingStack + offsetof(WorkStealingStack_t, bottom));
	}
	value_type* const values()
	{
		return reinterpret_cast<value_type* const>(workStealingStack + offsetof(WorkStealingStack_t, values));
	}
public:
	WorkStealingStackReference() {}

	template<size_t cap>
	WorkStealingStackReference(WorkStealingStack<value_type, cap>& other) : workStealingStack(reinterpret_cast<unsigned char*>(&other)), capacity(cap) {}

	template<size_t cap>
	void operator=(WorkStealingStack<value_type, cap>& other)
	{
		workStealingStack = reinterpret_cast<unsigned char*>(&other);
		capacity = cap;
	}

	// Single producer
	bool push(const value_type& item) noexcept {
		auto oldtop = top().load(std::memory_order_relaxed);
		auto oldbottom = bottom().load(std::memory_order_relaxed); //this thread is the only thread that changes bottom
		auto numtasks = oldbottom - oldtop;

		if (numtasks > capacity) {
			return false;
		}
		if (oldbottom == std::numeric_limits<unsigned long>::max()) { //bottum is going to overfrow
			auto newTop = top() % capacity;
			auto newBottum = oldbottom - top() + newTop;
			values()[newBottum % capacity] = item;
			bottom().store(newBottum + 1u, std::memory_order_release);
			top().store(newTop, std::memory_order_release);
		}
		else {
			values()[oldbottom % capacity] = item;
			bottom().store(oldbottom + 1u, std::memory_order_release); //other threads need to see changes to values
		}
		return true;
	}

	// Single consumer
	bool pop(value_type& result) noexcept {
		auto oldBottom = bottom().fetch_sub(1u, std::memory_order_release);
		auto oldTop = top().load(std::memory_order_acquire);

		auto newbottom = oldBottom - 1u;
		if (newbottom > oldTop)
		{
			result = values()[oldBottom % capacity];
			return true;
		}
		else if (newbottom == oldTop) //there was one element
		{
			if (top().compare_exchange_strong(oldTop, oldTop + 1u, std::memory_order_release, std::memory_order_relaxed))
			{
				//we got the job
				bottom().store(oldBottom, std::memory_order_release);
				result = values()[oldBottom % capacity];
				return true;
			}
			else//we were too late
			{
				bottom().store(oldBottom, std::memory_order_release);
				return false;
			}
		}
		return false;
	}

	// Multiple consumer.
	bool steal(value_type& result) noexcept {
		auto oldtop = top().load(std::memory_order_acquire);
		auto oldbottom = bottom().load(std::memory_order_acquire);

		if (oldbottom <= oldtop) return false;

		// Make sure that we are not contending for the item.
		if (!top().compare_exchange_strong(oldtop, oldtop + 1u, std::memory_order_release, std::memory_order_relaxed)) {
			return false;
		}

		result = values()[oldtop % capacity];
		return true;
	}
};