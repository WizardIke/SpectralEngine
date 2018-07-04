#pragma once
#include "Delegate.h"
#include "WorkStealingQueue.h"
#include "ThreadBarrier.h"
#include "IOCompletionQueue.h"
#include "SingleProducerSingleConsumerQueue.h"

template<class ThreadResources, class GlobalResources>
class TaskShedular
{
public:
	using Task = Delegate<void(ThreadResources& threadResources,
		GlobalResources& globalResoureces)>;
	using NonAtomicNextPhaseTask = bool(*)(ThreadResources& threadResources, GlobalResources& globalResoureces);
	using NextPhaseTask = std::atomic<NonAtomicNextPhaseTask>;

	class BackgroundQueue : public SingleProducerSingleConsumerQueue<Task, 256u>
	{
		std::atomic_flag locked = ATOMIC_FLAG_INIT;
	public:
		bool try_lock()
		{
			return !locked.test_and_set(std::memory_order::memory_order_acquire);
		}

		void unlock()
		{
			locked.clear(std::memory_order::memory_order_release);
		}
	};

	class ThreadLocal
	{
	private:
		WorkStealingQueue<Task> mUpdate1Queues[2];
		WorkStealingQueue<Task>* mUpdate1CurrentQueue;
		WorkStealingQueue<Task>* mUpdate1NextQueue;
		WorkStealingQueue<Task> mUpdate2Queues[2];
		WorkStealingQueue<Task>* mUpdate2CurrentQueue;
		WorkStealingQueue<Task>* mUpdate2NextQueue;
		WorkStealingQueue<Task>* mCurrentQueue;
		BackgroundQueue mBackgroundQueue;
	
		unsigned int mIndex;
		unsigned int mCurrentBackgroundQueueIndex;

		static unsigned int lockAndGetNextBackgroundQueue(unsigned int currentIndex, unsigned int threadCount, BackgroundQueue** backgroundQueues)
		{
			while (true)
			{
				++currentIndex;
				if (currentIndex == threadCount + 1u) { currentIndex = 0u; }
				if (currentIndex == threadCount) { return currentIndex; }
				if (backgroundQueues[currentIndex]->try_lock()) { return currentIndex; }
			}
		}

		void runBackgroundTasks(TaskShedular& taskShedular, ThreadResources& threadResources, GlobalResources& globalResoureces, unsigned int currentBackgroundQueueIndex)
		{
			if (currentBackgroundQueueIndex == taskShedular.mThreadCount)
			{
				bool found;
				IOCompletionPacket task;
				auto& queue = taskShedular.mIoCompletionQueue;
				while (true)
				{
					found = queue.pop(task);
					if (found)
					{
						task(&threadResources, &globalResoureces);
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				bool found;
				Task task;
				auto& queue = *taskShedular.mBackgroundQueues[currentBackgroundQueueIndex];
				while (true)
				{
					found = queue.pop(task);
					if (found)
					{
						task(threadResources, globalResoureces);
					}
					else
					{
						break;
					}
				}
				queue.unlock();
			}
		}

		template<void(*prepairForUpdate2)(ThreadResources& threadResources, GlobalResources& globalResoureces)>
		void getIntoCorrectStateAfterDoingBackgroundTasks(TaskShedular& taskShedular, ThreadResources& threadResources, GlobalResources& globalResoureces)
		{
			bool succeeded = taskShedular.mIncrementPrimaryThreadCountIfUpdateIndexIsZero();
			if (!succeeded)
			{
				for (unsigned int spin = 0u; spin != 1000u; ++spin)
				{
					succeeded = taskShedular.mIncrementPrimaryThreadCountIfUpdateIndexIsZero();
					if (succeeded) break;
				}
				while (!succeeded)
				{
					std::this_thread::yield();
					succeeded = taskShedular.mIncrementPrimaryThreadCountIfUpdateIndexIsZero();
				}
			}

			if (taskShedular.mCurrentWorkStealingQueues[mIndex] == mUpdate1CurrentQueue)
			{
				mUpdate2CurrentQueue->resetIfInvalid();
				mUpdate2NextQueue->resetIfInvalid();
				mUpdate1NextQueue->resetIfInvalid();

				auto temp = mUpdate2CurrentQueue;
				mUpdate2CurrentQueue = mUpdate2NextQueue;
				mUpdate2NextQueue = temp;

				mCurrentQueue = mUpdate1CurrentQueue;
			}
			else if (taskShedular.mCurrentWorkStealingQueues[mIndex] == mUpdate2CurrentQueue)
			{
				mUpdate2NextQueue->resetIfInvalid();
				mUpdate1CurrentQueue->resetIfInvalid();
				mUpdate1NextQueue->resetIfInvalid();

				prepairForUpdate2(threadResources, globalResoureces);
			}
			else if (taskShedular.mCurrentWorkStealingQueues[mIndex] == mUpdate1NextQueue)
			{
				mUpdate2CurrentQueue->resetIfInvalid();
				mUpdate2NextQueue->resetIfInvalid();
				mUpdate1CurrentQueue->resetIfInvalid();

				auto temp = mUpdate1CurrentQueue;
				mUpdate1CurrentQueue = mUpdate1NextQueue;
				mUpdate1NextQueue = temp;

				mCurrentQueue = mUpdate1CurrentQueue;
			}
			else //if(taskShedular.mCurrentWorkStealingQueues[mIndex] == mUpdate2NextQueue)
			{
				mUpdate2CurrentQueue->resetIfInvalid();
				mUpdate1CurrentQueue->resetIfInvalid();
				mUpdate1NextQueue->resetIfInvalid();

				auto temp = mUpdate1CurrentQueue;
				mUpdate1CurrentQueue = mUpdate1NextQueue;
				mUpdate1NextQueue = temp;
				temp = mUpdate2CurrentQueue;
				mUpdate2CurrentQueue = mUpdate2NextQueue;
				mUpdate2NextQueue = temp;

				mCurrentQueue = mUpdate2CurrentQueue;

				prepairForUpdate2(threadResources, globalResoureces);
			}
		}

		void endUpdate2End(TaskShedular& taskShedular, const unsigned int primaryThreadCount, const unsigned int updateIndex)
		{
			taskShedular.mBarrier.sync(primaryThreadCount, [&mUpdateIndexAndPrimaryThreadCount = taskShedular.mUpdateIndexAndPrimaryThreadCount,
				amount = taskShedular.mPrimaryThreadCountDefferedDecrease.load(std::memory_order::memory_order_relaxed)]()
			{
				unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order::memory_order_relaxed);
				mUpdateIndexAndPrimaryThreadCount.store((oldUpdateIndexAndPrimaryThreadCount - amount) & 0xffff, std::memory_order::memory_order_release);
			});

			if (updateIndex == primaryThreadCount - 1u)
			{
				taskShedular.mPrimaryThreadCountDefferedDecrease.store(0u, std::memory_order::memory_order_relaxed);
			}
			mUpdate2CurrentQueue->reset();
			auto temp = mUpdate1CurrentQueue;
			mUpdate1CurrentQueue = mUpdate1NextQueue;
			mUpdate1NextQueue = temp;
			mCurrentQueue = mUpdate1CurrentQueue;
		}
	public:
		ThreadLocal(unsigned int index, TaskShedular& taskShedular)
		{
			mIndex = index;
			mCurrentBackgroundQueueIndex = index;
			taskShedular.mBackgroundQueues[index] = &mBackgroundQueue;
			taskShedular.mWorkStealingQueuesArray[index] = &mUpdate1Queues[0];
			index += taskShedular.mThreadCount;
			taskShedular.mWorkStealingQueuesArray[index] = &mUpdate2Queues[0];
			index += taskShedular.mThreadCount;
			taskShedular.mWorkStealingQueuesArray[index] = &mUpdate1Queues[1];
			index += taskShedular.mThreadCount;
			taskShedular.mWorkStealingQueuesArray[index] = &mUpdate2Queues[1];

			mCurrentQueue = &mUpdate2Queues[0];
			mUpdate1CurrentQueue = &mUpdate1Queues[0];
			mUpdate1NextQueue = &mUpdate1Queues[1];
			mUpdate2CurrentQueue = &mUpdate2Queues[0];
			mUpdate2NextQueue = &mUpdate2Queues[1];
		}

		void sync(TaskShedular& taskShedular)
		{
			taskShedular.mBarrier.sync(taskShedular.primaryThreadCount());
		}
		
		WorkStealingQueue<Task>& update1CurrentQueue()
		{
			return *mUpdate1CurrentQueue;
		}
		WorkStealingQueue<Task>& update1NextQueue()
		{
			return *mUpdate1NextQueue;
		}
		
		WorkStealingQueue<Task>& update2CurrentQueue()
		{
			return *mUpdate2CurrentQueue;
		}
		WorkStealingQueue<Task>& update2NextQueue()
		{
			return *mUpdate2NextQueue;
		}

		SingleProducerSingleConsumerQueue<Task, 256u>& backgroundQueue()
		{
			return mBackgroundQueue;
		}

		unsigned int index() const
		{
			return mIndex;
		}
		
		void endUpdate1(TaskShedular& taskShedular, NonAtomicNextPhaseTask nextPhaseTask, const unsigned int updateIndex)
		{
			const unsigned int primaryThreadCount = taskShedular.primaryThreadCount();
			if (updateIndex == primaryThreadCount - 1u)
			{
				taskShedular.mNextPhaseTask.store(nextPhaseTask, std::memory_order::memory_order_relaxed);
				taskShedular.endUpdate1();
			}

			taskShedular.mBarrier.sync(primaryThreadCount, [&taskShedular]()
			{
				taskShedular.mResetUpdateIndex();
			});

			mUpdate1CurrentQueue->reset();
			auto temp = mUpdate2CurrentQueue;
			mUpdate2CurrentQueue = mUpdate2NextQueue;
			mUpdate2NextQueue = temp;
			mCurrentQueue = mUpdate2CurrentQueue;
		}

		void endUpdate2Primary(TaskShedular& taskShedular, NonAtomicNextPhaseTask nextPhaseTask, const unsigned int primaryThreadCount, const unsigned int updateIndex)
		{
			if (updateIndex == primaryThreadCount - 1u)
			{
				taskShedular.mNextPhaseTask.store(nextPhaseTask, std::memory_order::memory_order_relaxed);
				taskShedular.endUpdate2();
			}

			endUpdate2End(taskShedular, primaryThreadCount, updateIndex);
		}

		template<void(*endUpdate)(ThreadResources& threadResources, GlobalResources& globalResoureces), void(*prepairForUpdate2)(ThreadResources& threadResources, GlobalResources& globalResoureces)>
		void endUpdate2Background(TaskShedular& taskShedular, ThreadResources& threadResources, GlobalResources& globalResoureces, NonAtomicNextPhaseTask nextPhaseTask, const unsigned int primaryThreadCount, const unsigned int updateIndex)
		{
			if (updateIndex == primaryThreadCount - 1u)
			{
				taskShedular.mNextPhaseTask.store(nextPhaseTask, std::memory_order::memory_order_relaxed);
				taskShedular.endUpdate2();
			}

			unsigned int currentQueueIndex = lockAndGetNextBackgroundQueue(mCurrentBackgroundQueueIndex, taskShedular.mThreadCount, taskShedular.mBackgroundQueues);
			mCurrentBackgroundQueueIndex = currentQueueIndex;
			if (currentQueueIndex == taskShedular.mThreadCount)
			{
				IOCompletionPacket task;
				bool found = taskShedular.mIoCompletionQueue.pop(task);
				if (found)
				{
					taskShedular.mPrimaryThreadCountDefferedDecrease.fetch_add(1u, std::memory_order::memory_order_relaxed);
					taskShedular.mBarrier.sync(primaryThreadCount, [&mUpdateIndexAndPrimaryThreadCount = taskShedular.mUpdateIndexAndPrimaryThreadCount,
						amount = taskShedular.mPrimaryThreadCountDefferedDecrease.load(std::memory_order::memory_order_relaxed)]()
					{
						unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order::memory_order_relaxed);
						mUpdateIndexAndPrimaryThreadCount.store((oldUpdateIndexAndPrimaryThreadCount - amount) & 0xffff, std::memory_order::memory_order_release);
					});

					if (updateIndex == primaryThreadCount - 1u)
					{
						taskShedular.mPrimaryThreadCountDefferedDecrease.store(0u, std::memory_order::memory_order_relaxed);
					}
					mUpdate2CurrentQueue->reset();

					endUpdate(threadResources, globalResoureces);

					task(&threadResources, &globalResoureces);

					runBackgroundTasks(taskShedular, threadResources, globalResoureces, currentQueueIndex);
					getIntoCorrectStateAfterDoingBackgroundTasks<prepairForUpdate2>(taskShedular, threadResources, globalResoureces);
				}
				else
				{
					endUpdate2End(taskShedular, primaryThreadCount, updateIndex);
					endUpdate(threadResources, globalResoureces);
				}
			}
			else
			{
				Task task;
				bool found = taskShedular.mBackgroundQueues[currentQueueIndex]->pop(task);
				if (found)
				{
					taskShedular.mPrimaryThreadCountDefferedDecrease.fetch_add(1u, std::memory_order::memory_order_relaxed);
					taskShedular.mBarrier.sync(primaryThreadCount, [&mUpdateIndexAndPrimaryThreadCount = taskShedular.mUpdateIndexAndPrimaryThreadCount,
						amount = taskShedular.mPrimaryThreadCountDefferedDecrease.load(std::memory_order::memory_order_relaxed)]()
					{
						unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order::memory_order_relaxed);
						mUpdateIndexAndPrimaryThreadCount.store((oldUpdateIndexAndPrimaryThreadCount - amount) & 0xffff, std::memory_order::memory_order_release);
					});

					if (updateIndex == primaryThreadCount - 1u)
					{
						taskShedular.mPrimaryThreadCountDefferedDecrease.store(0u, std::memory_order::memory_order_relaxed);
					}
					mUpdate2CurrentQueue->reset();

					endUpdate(threadResources, globalResoureces);

					task(threadResources, globalResoureces);

					runBackgroundTasks(taskShedular, threadResources, globalResoureces, currentQueueIndex);
					getIntoCorrectStateAfterDoingBackgroundTasks<prepairForUpdate2>(taskShedular, threadResources, globalResoureces);
				}
				else
				{
					taskShedular.mBackgroundQueues[currentQueueIndex]->unlock();
					endUpdate2End(taskShedular, primaryThreadCount, updateIndex);
					endUpdate(threadResources, globalResoureces);
				}
			}
		}
		
		void start(TaskShedular& taskShedular, ThreadResources& threadResources,
			GlobalResources& globalResoureces)
		{
			auto currentWorkStealingQueues = taskShedular.mCurrentWorkStealingQueues;
			auto currentQueue = mCurrentQueue;
			while(true)
			{
				Task task;
				bool found = currentQueue->pop(task);
				if (found)
				{
					task(threadResources, globalResoureces);
				}
				else
				{
					for (auto i = mIndex + 1u;; ++i)
					{
						if(i == taskShedular.mThreadCount) i = 0u;
						found = currentWorkStealingQueues[i]->pop(task);
						if (found)
						{
							task(threadResources, globalResoureces);
							break;
						}
						else if (i == mIndex)
						{
							auto nextPhaseTask = taskShedular.mNextPhaseTask.load(std::memory_order::memory_order_relaxed);
							bool shouldQuit = nextPhaseTask(threadResources, globalResoureces);
							if(shouldQuit) return;
							currentWorkStealingQueues = taskShedular.mCurrentWorkStealingQueues;
							currentQueue = mCurrentQueue;
							break;
						}
					}
				}
			}
		}

		void runBackgroundTasks(TaskShedular& taskShedular, ThreadResources& threadResources, GlobalResources& globalResoureces)
		{
			unsigned int currentQueueIndex = lockAndGetNextBackgroundQueue(mCurrentBackgroundQueueIndex, taskShedular.mThreadCount, taskShedular.mBackgroundQueues);
			mCurrentBackgroundQueueIndex = currentQueueIndex;
			runBackgroundTasks(taskShedular, threadResources, globalResoureces, currentQueueIndex);
		}
	};
private:
	WorkStealingQueue<Task>** mWorkStealingQueuesArray;
	WorkStealingQueue<Task>** mCurrentWorkStealingQueues;
	BackgroundQueue** mBackgroundQueues;
	IOCompletionQueue mIoCompletionQueue;
	unsigned int mThreadCount;
	std::atomic<unsigned long> mUpdateIndexAndPrimaryThreadCount;
	std::atomic<unsigned int> mPrimaryThreadCountDefferedDecrease = 0u;
	NextPhaseTask mNextPhaseTask;
	ThreadBarrier mBarrier;

	bool mIncrementPrimaryThreadCountIfUpdateIndexIsZero()
	{
		unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order::memory_order_relaxed);
		while ((oldUpdateIndexAndPrimaryThreadCount & ~(unsigned long)0xffff) == 0u)
		{
			bool succeeded = mUpdateIndexAndPrimaryThreadCount.compare_exchange_weak(oldUpdateIndexAndPrimaryThreadCount, oldUpdateIndexAndPrimaryThreadCount + 1u,
				std::memory_order::memory_order_acquire, std::memory_order::memory_order_relaxed);
			if (succeeded) return true;
		}
		return false;
	}

	void mIncrementPrimaryThreadCount()
	{
		mUpdateIndexAndPrimaryThreadCount.fetch_add(1u, std::memory_order::memory_order_relaxed);
	}

	void mResetUpdateIndex()
	{
		unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order::memory_order_relaxed);
		mUpdateIndexAndPrimaryThreadCount.store(oldUpdateIndexAndPrimaryThreadCount & 0xffff, std::memory_order::memory_order_release);
	}

	void endUpdate1()
	{
		mCurrentWorkStealingQueues += mThreadCount;
	}
	
	void endUpdate2()
	{
		mCurrentWorkStealingQueues += mThreadCount;
		if(mCurrentWorkStealingQueues == (mWorkStealingQueuesArray + mThreadCount * 4u))
		{
			mCurrentWorkStealingQueues = mWorkStealingQueuesArray;
		}
	}
public:
	TaskShedular(unsigned int numberOfThreads, NonAtomicNextPhaseTask nextPhaseTask)
	{
		mWorkStealingQueuesArray = new WorkStealingQueue<Task>*[numberOfThreads * 4u];
		mBackgroundQueues = new BackgroundQueue*[numberOfThreads];
		mCurrentWorkStealingQueues = mWorkStealingQueuesArray + numberOfThreads;
		mThreadCount = numberOfThreads;
		mUpdateIndexAndPrimaryThreadCount.store(numberOfThreads, std::memory_order::memory_order_relaxed);
		mNextPhaseTask.store(nextPhaseTask, std::memory_order::memory_order_relaxed);
	}

	IOCompletionQueue& ioCompletionQueue()
	{
		return mIoCompletionQueue;
	}

	unsigned int threadCount()
	{
		return mThreadCount;
	}

	unsigned int primaryThreadCount()
	{
		return mUpdateIndexAndPrimaryThreadCount.load(std::memory_order::memory_order_relaxed) & 0xffff;
	}

	unsigned int incrementUpdateIndex()
	{
		unsigned long newUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.fetch_add(1ul << 16ul, std::memory_order::memory_order_relaxed);
		return (unsigned int)(newUpdateIndexAndPrimaryThreadCount >> 16ul);
	}

	unsigned int incrementUpdateIndex(unsigned int& primaryThreadCount)
	{
		unsigned long newUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.fetch_add(1ul << 16ul, std::memory_order::memory_order_relaxed);
		primaryThreadCount = (unsigned int)(newUpdateIndexAndPrimaryThreadCount & 0xffff);
		return (unsigned int)(newUpdateIndexAndPrimaryThreadCount >> 16ul);
	}

	NonAtomicNextPhaseTask getNextPhaseTask()
	{
		return mNextPhaseTask.load(std::memory_order_relaxed);
	}

	void setNextPhaseTask(NonAtomicNextPhaseTask nextPhaseTask)
	{
		mNextPhaseTask.store(nextPhaseTask, std::memory_order::memory_order_relaxed);
	}

	ThreadBarrier& barrier()
	{
		return mBarrier;
	}
};
