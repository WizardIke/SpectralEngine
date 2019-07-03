#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadBarrier
{
	std::mutex mutex;
	std::condition_variable conditionVariable;
	unsigned int generation = 0u;
	unsigned long waitingCount = 0u;
	std::atomic<unsigned long> threadCount;
	long increaseInThreads = 0u;

	void syncHelper(std::unique_lock<std::mutex> lock)
	{
		++waitingCount;
		auto oldThreadCount = threadCount.load(std::memory_order_relaxed) & 0x7fffffff;
		if (waitingCount == oldThreadCount)
		{
			threadCount.store(oldThreadCount + increaseInThreads, std::memory_order_relaxed); //unlock and increase by increaseInThreads
			increaseInThreads = 0u;
			waitingCount = 0u;
			++generation;
			lock.unlock();
			conditionVariable.notify_all();
		}
		else
		{
			conditionVariable.wait(lock, [&gen = generation, oldGen = generation]() { return gen != oldGen; });
			lock.unlock();
		}
	}
public:
	ThreadBarrier(unsigned long threadCount1) : threadCount(threadCount1) {}

	void sync()
	{
		syncHelper(std::unique_lock<std::mutex>(mutex));
	}

	void syncAndRemoveThread()
	{
		std::unique_lock<std::mutex> lock(mutex);
		--increaseInThreads;
		syncHelper(std::move(lock));
	}

	void addThread()
	{
		std::unique_lock<std::mutex> lock(mutex);
		bool threadCountIsLocked = false;
		auto oldThreadCount = threadCount.load(std::memory_order_relaxed);
		do
		{
			if ((oldThreadCount & 0x80000000) != 0)
			{
				threadCountIsLocked = true;
				break;
			}
		} while (threadCount.compare_exchange_weak(oldThreadCount, oldThreadCount + 1u, std::memory_order_relaxed, std::memory_order_relaxed));

		if (threadCountIsLocked)
		{
			++increaseInThreads;
			conditionVariable.wait(lock, [&gen = generation, oldGen = generation]() { return gen != oldGen; });
		}
	}

	unsigned int lockAndGetThreadCount()
	{
		auto oldThreadCount = threadCount.load(std::memory_order_relaxed);
		if ((oldThreadCount & 0x80000000) == 0)
		{
			//The threadCount is unlocked. Atomically lock it and get its value.
			oldThreadCount = threadCount.fetch_or(0x80000000, std::memory_order_relaxed);
		}
		return oldThreadCount & 0x7fffffff;
	}
};