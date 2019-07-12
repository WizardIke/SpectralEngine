#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadBarrier
{
	struct DoublyLinked
	{
		DoublyLinked* previous;
		DoublyLinked* next;
	};
public:
	class ThreadLocal : private DoublyLinked
	{
		friend class ThreadBarrier;
		ThreadLocal* previousToRemove;
	public:
		unsigned long index; //The index can change after sync is called and can be used as a unique index between calls to sync.

		ThreadLocal() {}
	};
private:
	std::mutex mutex;
	std::condition_variable conditionVariable;
	unsigned int generation = 0u;
	unsigned int waitingCount = 0u;
	std::atomic<unsigned long> threadCount;
	long increaseInThreads = 0u;
	ThreadLocal* threadsToRemove = nullptr;
	DoublyLinked threadLocals{ &threadLocals, &threadLocals }; //previous is end of list, next is start of list.

	template<class F>
	void syncHelper(std::unique_lock<std::mutex> lock, F&& f)
	{
		++waitingCount;
		auto oldThreadCount = threadCount.load(std::memory_order_relaxed) & 0x7fffffff;
		if (waitingCount == oldThreadCount)
		{
			while (threadsToRemove != nullptr)
			{
				auto threadLocalToReindex = threadLocals.previous;
				threadLocalToReindex->previous->next = &threadLocals;
				threadLocals.previous = threadLocalToReindex->previous;

				threadsToRemove->previous->next = threadLocalToReindex;
				threadsToRemove->next->previous = threadLocalToReindex;

				threadLocalToReindex->previous = threadsToRemove->previous;
				threadLocalToReindex->next = threadsToRemove->next;

				auto toReindex = static_cast<ThreadLocal*>(threadLocalToReindex);
				toReindex->index = threadsToRemove->index;
			
				threadsToRemove = threadsToRemove->previousToRemove;
				--increaseInThreads;
			}
			threadCount.store(oldThreadCount + increaseInThreads, std::memory_order_relaxed); //unlock and increase by increaseInThreads
			increaseInThreads = 0u;
			waitingCount = 0u;
			++generation;
			f();
			lock.unlock();
			conditionVariable.notify_all();
		}
		else
		{
			conditionVariable.wait(lock, [&gen = generation, oldGen = generation]() { return gen != oldGen; });
			lock.unlock();
		}
	}

	void start(std::unique_lock<std::mutex> lock)
	{
		++waitingCount;
		auto oldThreadCount = threadCount.load(std::memory_order_relaxed) & 0x7fffffff;
		if (waitingCount == oldThreadCount)
		{
			unsigned long index = waitingCount - increaseInThreads;
			while (threadsToRemove != nullptr)
			{
				threadsToRemove->index = index;
				threadsToRemove->previous = threadLocals.previous;
				threadsToRemove->next = &threadLocals;
				threadLocals.previous->next = threadsToRemove;
				threadLocals.previous = threadsToRemove;

				++index;
				threadsToRemove = threadsToRemove->previousToRemove;
			}
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
	ThreadBarrier(unsigned int threadCount1) : threadCount(threadCount1) {}

	/*
	Will have an index that never changes and is in the range [0, number of threads that can't leave) but can never call syncAndRemoveThread.
	*/
	void startCannotLeave(ThreadLocal& threadLocal)
	{
		std::unique_lock<std::mutex> lock(mutex);
		threadLocal.index = waitingCount - increaseInThreads;

		start(std::move(lock));
	}

	/*
	Can call syncAndRemoveThread and addThread but index can change after calling sync.
	*/
	void startCanLeave(ThreadLocal& threadLocal)
	{
		std::unique_lock<std::mutex> lock(mutex);
		++increaseInThreads;
		threadLocal.previousToRemove = threadsToRemove;
		threadsToRemove = &threadLocal;

		start(std::move(lock));
	}

	template<class F>
	void sync(F&& f)
	{
		syncHelper(std::unique_lock<std::mutex>(mutex), std::forward<F>(f));
	}

	template<class F>
	void syncAndRemoveThread(ThreadLocal& threadLocal, F&& f)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (threadLocal.next == &threadLocals)
		{
			//threadLocal is last in threadLocals so it can be removed without changing other indices.
			--increaseInThreads;
			threadLocal.previous->next = threadLocal.next;
			threadLocal.next->previous = threadLocal.previous;
		}
		else
		{
			//threadLocal isn't last in threadLocals so it can't be removed without changing the last index which can't be changed yet as it might be in use by another thread.
			threadLocal.previousToRemove = threadsToRemove;
			threadsToRemove = &threadLocal;
		}
		syncHelper(std::move(lock), std::forward<F>(f));
	}

	void addThread(ThreadLocal& threadLocal)
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
		} while (!threadCount.compare_exchange_weak(oldThreadCount, oldThreadCount + 1u, std::memory_order_relaxed, std::memory_order_relaxed));

		threadLocal.index = (oldThreadCount & 0x7fffffff) + increaseInThreads;
		threadLocal.previous = threadLocals.previous;
		threadLocal.next = &threadLocals;
		threadLocals.previous->next = &threadLocal;
		threadLocals.previous = &threadLocal;

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
		return static_cast<unsigned int>(oldThreadCount & 0x7fffffff);
	}

	template<class F>
	void stop(unsigned int threadCount, F&& f)
	{
		std::unique_lock<std::mutex> lock(mutex);
		++waitingCount;
		if (waitingCount == threadCount)
		{
			f();
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
};