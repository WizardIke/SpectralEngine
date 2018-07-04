#include "ThreadBarrier.h"

void ThreadBarrier::sync(unsigned int maxThreads)
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
		conditionVariable.wait(lock, [&gen = mGeneration, oldGen = mGeneration]() { return gen != oldGen; });
		lock.unlock();
	}
}

void ThreadBarrier::sync(std::unique_lock<std::mutex>&& lock, unsigned int maxThreads)
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
		conditionVariable.wait(lock, [&gen = mGeneration, oldGen = mGeneration]() { return gen != oldGen; });
		lock.unlock();
	}
}