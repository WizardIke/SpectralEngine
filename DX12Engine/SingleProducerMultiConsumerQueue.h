#pragma once
#include <cstddef>
#include <atomic>

template<class T>
class SingleProducerMultiConsumerQueue
{
	T* mData;
	std::atomic<std::size_t> mEnd;
	std::size_t mFront;
	std::size_t mCapacity;
public:
	//size must be a power of two
	SingleProducerMultiConsumerQueue(std::size_t capacity)
	{
		mEnd = 0;
		mFront = 0;
		mCapacity = capacity;
		mData = new T[capacity];
	}
	
	~SingleProducerMultiConsumerQueue()
	{
		delete[] mData;
	}
	
	void push(T item)
	{
		std::size_t oldEnd = mEnd.load(std::memory_order_relaxed);
		mData[oldEnd & (mCapacity - 1u)] = std::move(item);
		mEnd.store(oldEnd + 1u, std::memory_order_release);
	}
	
	bool pop(T& item)
	{
		std::size_t oldEnd;
		std::size_t oldFront = mFront.load(std::memory_order_relaxed);
		do
		{
			oldEnd = mEnd.load(std::memory_order_acquire);
			if(oldFront == oldEnd) return false;//the queue was probably empty
		} while(!mFront.compare_exchange_weak(oldFront, oldFront + 1u, std::memory_order_relaxed));
		item = ((const T*)mData)[oldFront & (mCapacity - 1u)];
	}
};