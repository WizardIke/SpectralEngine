#pragma once
#include "WorkStealingQueue.h"
#include "BackgroundQueue.h"
#include "PrimaryTaskFromOtherThreadQueue.h"
#include "Delegate.h"
#include "ThreadBarrier.h"
#include "Array.h"
#include <cstddef>
#include <cassert>
#include <atomic>
#include <memory>

template<class ThreadResources, class GlobalResources>
class TaskShedular
{
private:
	constexpr static std::size_t numberOfPrimaryQueues = 2u; //can't change this yet as we still have update1 and update2
public:
	using Task = Delegate<void(ThreadResources& threadResources,
		GlobalResources& globalResoureces)>;
	using NextPhaseTask = bool(*)(ThreadResources& threadResources, GlobalResources& globalResoureces);

	class ThreadLocal
	{
	private:
		class PrimaryQueue
		{
		public:
			WorkStealingQueue<Task> queues[2];
			WorkStealingQueue<Task>* currentQueue;
			WorkStealingQueue<Task>* nextQueue;
		};
		PrimaryQueue primaryQueues[numberOfPrimaryQueues];
		WorkStealingQueue<Task>* mCurrentQueue;
		BackgroundQueue<Task> mBackgroundQueue;
	
		unsigned int mIndex;
		unsigned int mCurrentBackgroundQueueIndex;

		void swap(WorkStealingQueue<Task>*& lhs, WorkStealingQueue<Task>*& rhs)
		{
			auto temp = lhs;
			lhs = rhs;
			rhs = temp;
		}

		static unsigned int lockAndGetNextBackgroundQueue(unsigned int currentIndex, unsigned int threadCount, BackgroundQueue<Task>** backgroundQueues)
		{
			while (true)
			{
				++currentIndex;
				if (currentIndex == threadCount) { currentIndex = 0u; }
				if (backgroundQueues[currentIndex]->try_lock()) { return currentIndex; }
			}
		}

		void runBackgroundTasks(TaskShedular& taskShedular, ThreadResources& threadResources, GlobalResources& globalResoureces, unsigned int currentBackgroundQueueIndex)
		{
			Task task;
			auto& queue = *taskShedular.mBackgroundQueues[currentBackgroundQueueIndex];
			while (queue.pop(task))
			{
				task(threadResources, globalResoureces);
			}
			queue.unlock();
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

			if (taskShedular.mCurrentWorkStealingQueues[mIndex] == primaryQueues[0].currentQueue)
			{
				primaryQueues[1].currentQueue->resetIfInvalid();
				primaryQueues[1].nextQueue->resetIfInvalid();
				primaryQueues[0].nextQueue->resetIfInvalid();

				swap(primaryQueues[1].currentQueue, primaryQueues[1].nextQueue);

				mCurrentQueue = primaryQueues[0].currentQueue;
			}
			else if (taskShedular.mCurrentWorkStealingQueues[mIndex] == primaryQueues[1].currentQueue)
			{
				primaryQueues[1].nextQueue->resetIfInvalid();
				primaryQueues[0].currentQueue->resetIfInvalid();
				primaryQueues[0].nextQueue->resetIfInvalid();

				prepairForUpdate2(threadResources, globalResoureces);
			}
			else if (taskShedular.mCurrentWorkStealingQueues[mIndex] == primaryQueues[0].nextQueue)
			{
				primaryQueues[1].currentQueue->resetIfInvalid();
				primaryQueues[1].nextQueue->resetIfInvalid();
				primaryQueues[0].currentQueue->resetIfInvalid();

				swap(primaryQueues[0].currentQueue, primaryQueues[0].nextQueue);

				mCurrentQueue = primaryQueues[0].currentQueue;
			}
			else //if(taskShedular.mCurrentWorkStealingQueues[mIndex] == primaryQueues[1].nextQueue)
			{
				primaryQueues[1].currentQueue->resetIfInvalid();
				primaryQueues[0].currentQueue->resetIfInvalid();
				primaryQueues[0].nextQueue->resetIfInvalid();

				swap(primaryQueues[0].currentQueue, primaryQueues[0].nextQueue);
				swap(primaryQueues[1].currentQueue, primaryQueues[1].nextQueue);

				mCurrentQueue = primaryQueues[1].currentQueue;

				prepairForUpdate2(threadResources, globalResoureces);
			}
		}

		void endUpdate2Sync(TaskShedular& taskShedular, const unsigned int primaryThreadCount)
		{
			taskShedular.mBarrier.sync(primaryThreadCount, [&mUpdateIndexAndPrimaryThreadCount = taskShedular.mUpdateIndexAndPrimaryThreadCount,
				&primaryThreadCountDefferedDecrease = taskShedular.mPrimaryThreadCountDefferedDecrease]()
			{
				unsigned int amount = primaryThreadCountDefferedDecrease.load(std::memory_order_relaxed);
				unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order_relaxed);
				mUpdateIndexAndPrimaryThreadCount.store((oldUpdateIndexAndPrimaryThreadCount - amount) & 0xffff, std::memory_order_release);
				primaryThreadCountDefferedDecrease.store(0u, std::memory_order_relaxed);
			});
		}
	public:
		ThreadLocal(unsigned int index, TaskShedular& taskShedular)
		{
			mIndex = index;
			mCurrentBackgroundQueueIndex = index;
			taskShedular.mBackgroundQueues[index] = &mBackgroundQueue;
			const auto threadCount = taskShedular.mThreadCount;
			for(auto& queue : primaryQueues)
			{
				taskShedular.mWorkStealingQueuesArray[index] = &queue.queues[0];
				index += threadCount;
			}
			for(auto& queue : primaryQueues)
			{
				taskShedular.mWorkStealingQueuesArray[index] = &queue.queues[1];
				index += threadCount;
			}

			mCurrentQueue = &primaryQueues[0u].queues[0u];
			primaryQueues[0u].currentQueue = &primaryQueues[0u].queues[0u];
			primaryQueues[0u].nextQueue = &primaryQueues[0u].queues[1u];
			for (std::size_t i = 1u; i != numberOfPrimaryQueues; ++i)
			{
				primaryQueues[i].currentQueue = &primaryQueues[i].queues[1u];
				primaryQueues[i].nextQueue = &primaryQueues[i].queues[0u];
			}
		}

		void sync(TaskShedular& taskShedular)
		{
			taskShedular.mBarrier.sync(taskShedular.threadCount());
		}

		/*
		Must be called from primary thread
		*/
		void pushPrimaryTask(std::size_t index, Task task)
		{
			assert(index < numberOfPrimaryQueues);
			primaryQueues[index].nextQueue->push(std::move(task));
		}

		void pushBackgroundTask(Task item)
		{
			mBackgroundQueue.push(std::move(item));
		}

		unsigned int index() const noexcept
		{
			return mIndex;
		}
		
		void endUpdate1(TaskShedular& taskShedular, NextPhaseTask nextPhaseTask, const unsigned int updateIndex, const unsigned int primaryThreadCount) noexcept
		{
			if (updateIndex == primaryThreadCount - 1u)
			{
				taskShedular.mNextPhaseTask = nextPhaseTask;
				taskShedular.endUpdate1();
			}

			taskShedular.mBarrier.sync(primaryThreadCount, [&taskShedular]()
			{
				taskShedular.mResetUpdateIndex();
			});

			primaryQueues[0].currentQueue->reset();
			swap(primaryQueues[1].currentQueue, primaryQueues[1].nextQueue);
			mCurrentQueue = primaryQueues[1].currentQueue;
		}

		void beforeEndUpdate2(TaskShedular& taskShedular, NextPhaseTask nextPhaseTask, const unsigned int primaryThreadCount, const unsigned int updateIndex)
		{
			if (updateIndex == primaryThreadCount - 1u)
			{
				taskShedular.mNextPhaseTask = nextPhaseTask;
				taskShedular.endUpdate2();
			}
		}

		void endUpdate2Primary(TaskShedular& taskShedular, const unsigned int primaryThreadCount)
		{
			endUpdate2Sync(taskShedular, primaryThreadCount);

			primaryQueues[1].currentQueue->reset();
			swap(primaryQueues[0].currentQueue, primaryQueues[0].nextQueue);
			mCurrentQueue = primaryQueues[0].currentQueue;
		}

		template<void(*prepairForUpdate2)(ThreadResources& threadResources, GlobalResources& globalResoureces)>
		void endUpdate2Background(TaskShedular& taskShedular, ThreadResources& threadResources, GlobalResources& globalResoureces, const unsigned int primaryThreadCount)
		{
			unsigned int currentQueueIndex = lockAndGetNextBackgroundQueue(mCurrentBackgroundQueueIndex, taskShedular.mThreadCount, taskShedular.mBackgroundQueues);
			mCurrentBackgroundQueueIndex = currentQueueIndex;
			
			Task task;
			if (taskShedular.mBackgroundQueues[currentQueueIndex]->pop(task))
			{
				taskShedular.mPrimaryThreadCountDefferedDecrease.fetch_add(1u, std::memory_order_relaxed);
				endUpdate2Sync(taskShedular, primaryThreadCount);

				primaryQueues[1].currentQueue->reset();

				task(threadResources, globalResoureces);

				runBackgroundTasks(taskShedular, threadResources, globalResoureces, currentQueueIndex);
				getIntoCorrectStateAfterDoingBackgroundTasks<prepairForUpdate2>(taskShedular, threadResources, globalResoureces);
			}
			else
			{
				taskShedular.mBackgroundQueues[currentQueueIndex]->unlock();
				endUpdate2Primary(taskShedular, primaryThreadCount);
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
							const auto nextPhaseTask = taskShedular.mNextPhaseTask;
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
	BackgroundQueue<Task>** mBackgroundQueues;
	unsigned int mThreadCount;
	std::atomic<unsigned long> mUpdateIndexAndPrimaryThreadCount;
	std::atomic<unsigned int> mPrimaryThreadCountDefferedDecrease = 0u;
	NextPhaseTask mNextPhaseTask;
	ThreadBarrier mBarrier;
	Array<PrimaryTaskFromOtherThreadQueue, numberOfPrimaryQueues> primaryFromOtherThreadQueues;

	bool mIncrementPrimaryThreadCountIfUpdateIndexIsZero() noexcept
	{
		unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order_relaxed);
		while ((oldUpdateIndexAndPrimaryThreadCount & ~(unsigned long)0xffff) == 0u)
		{
			bool succeeded = mUpdateIndexAndPrimaryThreadCount.compare_exchange_weak(oldUpdateIndexAndPrimaryThreadCount, oldUpdateIndexAndPrimaryThreadCount + 1u,
				std::memory_order_acquire, std::memory_order_relaxed);
			if (succeeded) return true;
		}
		return false;
	}

	void mResetUpdateIndex() noexcept
	{
		unsigned long oldUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.load(std::memory_order_relaxed);
		mUpdateIndexAndPrimaryThreadCount.store(oldUpdateIndexAndPrimaryThreadCount & 0xffff, std::memory_order_release);
	}

	void endUpdate1() noexcept
	{
		mCurrentWorkStealingQueues += mThreadCount;
	}
	
	void endUpdate2() noexcept
	{
		mCurrentWorkStealingQueues += mThreadCount;
		if(mCurrentWorkStealingQueues == (mWorkStealingQueuesArray + mThreadCount * 4u))
		{
			mCurrentWorkStealingQueues = mWorkStealingQueuesArray;
		}
	}
public:
	TaskShedular(unsigned int numberOfThreads, NextPhaseTask nextPhaseTask) : primaryFromOtherThreadQueues{[](std::size_t i, PrimaryTaskFromOtherThreadQueue& element)
	{
		new(&element) PrimaryTaskFromOtherThreadQueue(i);
	}},
		mNextPhaseTask(nextPhaseTask)
	{
		mWorkStealingQueuesArray = new WorkStealingQueue<Task>*[numberOfThreads * 2u * numberOfPrimaryQueues];
		mBackgroundQueues = new BackgroundQueue<Task>*[numberOfThreads];
		mCurrentWorkStealingQueues = mWorkStealingQueuesArray;
		mThreadCount = numberOfThreads;
		mUpdateIndexAndPrimaryThreadCount.store(numberOfThreads, std::memory_order_relaxed);
	}

	unsigned int threadCount() noexcept
	{
		return mThreadCount;
	}

	unsigned int incrementUpdateIndex(unsigned int& primaryThreadCount) noexcept
	{
		unsigned long newUpdateIndexAndPrimaryThreadCount = mUpdateIndexAndPrimaryThreadCount.fetch_add(1ul << 16ul, std::memory_order_acq_rel);
		primaryThreadCount = (unsigned int)(newUpdateIndexAndPrimaryThreadCount & 0xfffful);
		return (unsigned int)(newUpdateIndexAndPrimaryThreadCount >> 16ul);
	}

	NextPhaseTask getNextPhaseTask() noexcept
	{
		return mNextPhaseTask;
	}

	/*
	Must be called will care!!! Can only be called from very specific parts of the frame.
	*/
	void setNextPhaseTask(NextPhaseTask nextPhaseTask) noexcept
	{
		mNextPhaseTask = nextPhaseTask;
	}

	template<class OnLastThread>
	void sync(OnLastThread&& onLastThread)
	{
		mBarrier.sync(mThreadCount, std::forward<OnLastThread>(onLastThread));
	}

	/*
		Can be called from any thread
	*/
	void pushPrimaryTaskFromOtherThread(std::size_t index, PrimaryTaskFromOtherThreadQueue::Task& task) noexcept
	{
		assert(index < numberOfPrimaryQueues);
		primaryFromOtherThreadQueues[index].push(task);
	}

	void start(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		for(std::size_t i = 0u; i != numberOfPrimaryQueues; ++i)
		{
			primaryFromOtherThreadQueues[i].start(threadResources, globalResources);
		}
	}

	class StopRequest
	{
		friend class TaskShedular<ThreadResources, GlobalResources>;
		using Self = StopRequest;

		class PrimaryFromOtherStopRequest : public PrimaryTaskFromOtherThreadQueue::StopRequest
		{
			using Base = PrimaryTaskFromOtherThreadQueue::StopRequest;
		public:
			Self* stopRequest;

			PrimaryFromOtherStopRequest(void(*callback1)(Base& stopRequest, void* tr, void* gr), Self* stopRequest1) :
				Base(callback1),
				stopRequest(stopRequest1) {}
		};

		constexpr static std::size_t numberOfComponents = numberOfPrimaryQueues;
		std::atomic<std::size_t> numberOfComponentsStopped = 0u;
		Array<PrimaryFromOtherStopRequest, numberOfPrimaryQueues> primaryFromOtherStopRequests;
		void(*callback)(StopRequest& stopRequest, void* tr, void* gr);

		static void componentStopped(PrimaryTaskFromOtherThreadQueue::StopRequest& request, void* tr, void* gr)
		{
			auto& stopRequest = *static_cast<PrimaryFromOtherStopRequest&>(request).stopRequest;
			if(stopRequest.numberOfComponentsStopped.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
			{
				stopRequest.callback(stopRequest, tr, gr);
			}
		}
	public:
		StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr, void* gr)) : callback(callback1),
			primaryFromOtherStopRequests{[this](std::size_t, PrimaryFromOtherStopRequest& element)
		{
			new(&element) PrimaryFromOtherStopRequest(componentStopped, this);
		}} {}
	};

	void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		for(std::size_t i = 0u; i != numberOfPrimaryQueues; ++i)
		{
			primaryFromOtherThreadQueues[i].stop(stopRequest.primaryFromOtherStopRequests[i], threadResources, globalResources);
		}
	}
};
