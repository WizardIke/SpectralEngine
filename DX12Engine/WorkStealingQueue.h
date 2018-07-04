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
		if(capacity <= 0) capacity = 1;
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

	//can only be executed by the thread that owns this queue.
	//cannot resize the queue
	//can be called while another thread is popping items off the queue
	void concurrentPush(const T& item)
	{
		std::ptrdiff_t oldEndIndex = mEndIndex.load(std::memory_order_relaxed);
		while (oldEndIndex > 0u)
		{
			std::ptrdiff_t newEndIndex = oldEndIndex + 1;
			new(mBegin + oldEndIndex) T(item);
			if (mEndIndex.compare_exchange_weak(oldEndIndex, newEndIndex, std::memory_order::memory_order_release, std::memory_order::memory_order_relaxed))
			{
				return;
			}
			mBegin[newEndIndex - 1u].~T();
		}
		new(mBegin) T(item);
		mEndIndex.store(1u, std::memory_order::memory_order_release);
	}
	
	//can be called by any thread.
	//can only be called after the memory effects of all pushes are visable.
	bool pop(T& item)
	{
		const std::ptrdiff_t oldEndIndex = mEndIndex.fetch_sub(1, std::memory_order::memory_order_acquire) - 1;
		if(oldEndIndex < 0) return false;
		item = std::move(mBegin[oldEndIndex]);
		mBegin[oldEndIndex].~T();
		return true;
	}
	
	//reset or resetIfInvalid must be called after poping has finished and before pushing starts.
	//can only be executed by the thread that owns this queue.
	void reset()
	{
		mEndIndex.store(0, std::memory_order_relaxed);
	}

	//resets the queue if it doesn't contain items.
	//can only be executed by the thread that owns this queue.
	void resetIfInvalid()
	{
		std::ptrdiff_t oldEndIndex = mEndIndex.load(std::memory_order_relaxed);
		if (oldEndIndex < 0)
		{
			mEndIndex.store(0, std::memory_order_relaxed);
		}
	}
};