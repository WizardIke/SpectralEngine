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
	void sync(unsigned int maxThreads);

	template<class OnLastThread>
	void sync(unsigned int maxThreads, OnLastThread onLastThread)
	{
		std::unique_lock<std::mutex> lock(mMutex);
		++mWaitingCount;
		if (mWaitingCount == maxThreads)
		{
			onLastThread();
			mWaitingCount = 0u;
			++mGeneration;
			lock.unlock();
			conditionVariable.notify_all();
		}
		else
		{
			conditionVariable.wait(lock, [&gen = mGeneration, oldGen = mGeneration]() { return gen != oldGen; });
			lock.unlock();
		}
	}
};