#pragma once
#include <atomic>

template<class T, size_t mCapacity>
class SingleProducerSingleConsumerQueue
{
	T* mFront;
	T mData[mCapacity];
	std::atomic<T*> mBack;
public:
	SingleProducerSingleConsumerQueue()
	{
		mFront = mData;
		mBack = mData;
	}

	void push(T item)
	{
		T* oldBack = mBack.load(std::memory_order::memory_order_relaxed);
		*oldBack = std::move(item);
		T* newBack = oldBack + 1u;
		if (newBack == mData + mCapacity) { newBack = mData; }
		mBack.store(newBack, std::memory_order::memory_order_release);
	}

	bool pop(T& item)
	{
		T* oldBack = mBack.load(std::memory_order::memory_order_acquire);
		if (mFront != oldBack)
		{
			item = std::move(*mFront);
			++mFront;
			if (mFront == mData + mCapacity) { mFront = mData; }
			return true;
		}
		return false;
	}
};