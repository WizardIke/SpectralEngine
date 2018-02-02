#pragma once
#include <mutex>
#include <condition_variable>

class ThreadBarrier
{
	std::mutex mMutex;
	std::condition_variable conditionVariable;
	unsigned int mWaitingCount = 0u;
	unsigned int mGeneration = 0u;
public:
	void lock()
	{
		mMutex.lock();
	}

	bool try_lock()
	{
		return mMutex.try_lock();
	}

	void unlock()
	{
		mMutex.unlock();
	}

	void sync(unsigned int maxThreads)
	{
		std::unique_lock<decltype(mMutex)> lock(mMutex);
		++mWaitingCount;
		if (mWaitingCount == maxThreads)
		{
			mWaitingCount = 0u;
			++mGeneration;
			lock.unlock();
			conditionVariable.notify_all();
		}
		else
		{
			conditionVariable.wait(lock);
			lock.unlock();
		}
	}

	void sync(std::unique_lock<std::mutex>&& lock, unsigned int maxThreads)
	{
		++mWaitingCount;
		if (mWaitingCount == maxThreads)
		{
			mWaitingCount = 0u;
			++mGeneration;
			lock.unlock();
			conditionVariable.notify_all();
		}
		else
		{
			conditionVariable.wait(lock);
			lock.unlock();
		}
	}

	template<class LastThread, class NotLastThread>
	void sync(std::unique_lock<std::mutex>&& lock, unsigned int maxThreads, LastThread lastThread, NotLastThread notLastThread)
	{
		++mWaitingCount;
		if (mWaitingCount == maxThreads)
		{
			onLastThread();
			mWaitingCount = 0u;
			++mGeneration;
			conditionVariable.notify_all();
		}
		else
		{
			notLastThread();
			conditionVariable.wait(lock);
		}
		lock.unlock();
	}

	void wait(std::unique_lock<decltype(mMutex)>& lock)
	{
		conditionVariable.wait(lock);
	}

	template<class F>
	void wait(std::unique_lock<decltype(mMutex)>& lock, F f)
	{
		conditionVariable.wait(lock, f);
	}

	void notify_all()
	{
		conditionVariable.notify_all();
	}

	unsigned int& waiting_count()
	{
		return mWaitingCount;
	}

	std::mutex& mutex()
	{
		return mMutex;
	}

	unsigned int& generation()
	{
		return mGeneration;
	}
};