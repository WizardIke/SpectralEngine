#pragma once
#include <atomic>
#include <memory>
#include <cstddef>
#include <cassert>

template<class T, class Alloc = std::allocator<T>>
class WorkStealingQueue : private Alloc
{
	T* mBegin;
	std::atomic<std::ptrdiff_t> mEndIndex;
	std::ptrdiff_t mCapacity;

	void resize()
	{
		std::ptrdiff_t newCapacity = mCapacity * 2;
		T* newBegin = this->allocate(newCapacity);
		for (std::ptrdiff_t i = 0; i != mCapacity; ++i)
		{
			new(newBegin + i) T(std::move(mBegin[i]));
			mBegin[i].~T();
		}
		this->deallocate(mBegin, mCapacity);
		mBegin = newBegin;
		mCapacity = newCapacity;
	}
public:
	WorkStealingQueue() : mEndIndex(0), mCapacity(128u)
	{
		mBegin = this->allocate(128u);
	}
	
	WorkStealingQueue(std::ptrdiff_t capacity) : mEndIndex(0)
	{
		if(capacity <= 0) capacity = 8;
		mCapacity = capacity;
		mBegin = this->allocate(capacity);
	}
	
	~WorkStealingQueue()
	{
		this->deallocate(mBegin, mCapacity);
	}
	
	//can only be executed by the thread that owns this queue.
	//can only be called after the memory effects of all pops are visable and reset/resetIfInvalid has been called.
	void push(T item)
	{
		std::ptrdiff_t oldEndIndex = mEndIndex.load(std::memory_order_relaxed);
		std::ptrdiff_t newEndIndex = oldEndIndex + 1;
		if(newEndIndex == mCapacity)
		{
			resize();
		}
		new(mBegin + oldEndIndex) T(std::move(item));
		mEndIndex.store(newEndIndex, std::memory_order_relaxed);
	}
	
	//can be called by any thread.
	//can only be called after the memory effects of all pushes are visable.
	bool pop(T& item)
	{
		const std::ptrdiff_t oldEndIndex = mEndIndex.fetch_sub(1, std::memory_order_relaxed) - 1;
		if(oldEndIndex < 0) return false;
		item = std::move(mBegin[oldEndIndex]);
		mBegin[oldEndIndex].~T();
		return true;
	}
	
	//reset or resetIfInvalid must be called after poping has finished and before pushing starts.
	//can only be executed by the thread that owns this queue.
	void reset() noexcept
	{
		mEndIndex.store(0, std::memory_order_relaxed);
	}

	//resets the queue if it doesn't contain items.
	//can only be executed by the thread that owns this queue.
	void resetIfInvalid() noexcept
	{
		std::ptrdiff_t oldEndIndex = mEndIndex.load(std::memory_order_relaxed);
		if (oldEndIndex < 0)
		{
			mEndIndex.store(0, std::memory_order_relaxed);
		}
	}
};