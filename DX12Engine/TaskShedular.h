#pragma once
#include "WorkStealingQueue.h"
#include "BackgroundQueue.h"
#include "PrimaryTaskFromOtherThreadQueue.h"
#include "ThreadBarrier.h"
#include "Delegate.h"
#include <array>
#include "makeArray.h"
#include <cstddef>
#include <cassert>
#include <atomic>
#include <memory>

template<class ThreadResources>
class TaskShedular
{
private:
	constexpr static std::size_t numberOfPrimaryQueues = 2u; //can't change this yet as we still have update1 and update2
public:
	using Task = Delegate<void(ThreadResources& threadResources)>;
	using NextPhaseTask = bool(*)(ThreadResources& threadResources, void* context);

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

		ThreadBarrier::ThreadLocal barrier;
	
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

		void runBackgroundTasks(TaskShedular& taskShedular, ThreadResources& threadResources, unsigned int currentBackgroundQueueIndex)
		{
			Task task;
			auto& queue = *taskShedular.mBackgroundQueues[currentBackgroundQueueIndex];
			while (queue.pop(task))
			{
				task(threadResources);
			}
			queue.unlock();
		}

		template<void(*prepairForUpdate2)(ThreadResources& threadResources, void* context)>
		void getIntoCorrectStateAfterDoingBackgroundTasks(TaskShedular& taskShedular, ThreadResources& threadResources, void* context)
		{
			taskShedular.barrier.addThread(barrier);

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

				prepairForUpdate2(threadResources, context);
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

				prepairForUpdate2(threadResources, context);
			}
		}

		void start(TaskShedular& taskShedular, ThreadResources& threadResources, void* context)
		{
			auto currentWorkStealingQueues = taskShedular.mCurrentWorkStealingQueues;
			auto currentQueue = mCurrentQueue;
			while (true)
			{
				Task task;
				bool found = currentQueue->pop(task);
				if (found)
				{
					task(threadResources);
				}
				else
				{
					for (auto i = mIndex + 1u;; ++i)
					{
						if (i == taskShedular.mThreadCount) i = 0u;
						if (i == mIndex)
						{
							const auto nextPhaseTask = taskShedular.mNextPhaseTask;
							bool shouldQuit = nextPhaseTask(threadResources, context);
							if (shouldQuit) return;
							currentWorkStealingQueues = taskShedular.mCurrentWorkStealingQueues;
							currentQueue = mCurrentQueue;
							break;
						}
						found = currentWorkStealingQueues[i]->pop(task);
						if (found)
						{
							task(threadResources);
							break;
						}
					}
				}
			}
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

		template<class F>
		void stop(TaskShedular& taskShedular, F&& f)
		{
			taskShedular.barrier.stop(taskShedular.mThreadCount, std::forward<F>(f));
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

		/*
		An index in the range [0, threadCount)
		*/
		unsigned int index() const noexcept
		{
			return mIndex;
		}

		/*
		An index in the range [0, primaryThreadCount)
		*/
		unsigned int primaryIndex() const noexcept
		{
			return barrier.index;
		}
		
		void endUpdate1(TaskShedular& taskShedular, NextPhaseTask nextPhaseTask) noexcept
		{
			taskShedular.barrier.sync([&taskShedular, nextPhaseTask]()
				{
					taskShedular.mNextPhaseTask = nextPhaseTask;
					taskShedular.endUpdate<0>();
				});

			primaryQueues[0].currentQueue->reset();
			swap(primaryQueues[1].currentQueue, primaryQueues[1].nextQueue);
			mCurrentQueue = primaryQueues[1].currentQueue;
		}

		void endUpdate2Main(TaskShedular& taskShedular, NextPhaseTask nextPhaseTask)
		{
			taskShedular.mNextPhaseTask = nextPhaseTask;
			taskShedular.endUpdate<1>();
			endUpdate2Primary(taskShedular);
		}

		void endUpdate2Primary(TaskShedular& taskShedular)
		{
			taskShedular.barrier.sync([](){});

			primaryQueues[1].currentQueue->reset();
			swap(primaryQueues[0].currentQueue, primaryQueues[0].nextQueue);
			mCurrentQueue = primaryQueues[0].currentQueue;
		}

		template<void(*prepairForUpdate2)(ThreadResources& threadResources, void* context)>
		void endUpdate2Background(TaskShedular& taskShedular, ThreadResources& threadResources, void* context)
		{
			unsigned int currentQueueIndex = lockAndGetNextBackgroundQueue(mCurrentBackgroundQueueIndex, taskShedular.mThreadCount, taskShedular.mBackgroundQueues.get());
			mCurrentBackgroundQueueIndex = currentQueueIndex;
			
			Task task;
			if (taskShedular.mBackgroundQueues[currentQueueIndex]->pop(task))
			{
				taskShedular.barrier.syncAndRemoveThread(barrier, [](){});

				primaryQueues[1].currentQueue->reset();

				task(threadResources);

				runBackgroundTasks(taskShedular, threadResources, currentQueueIndex);
				getIntoCorrectStateAfterDoingBackgroundTasks<prepairForUpdate2>(taskShedular, threadResources, context);
			}
			else
			{
				taskShedular.mBackgroundQueues[currentQueueIndex]->unlock();
				endUpdate2Primary(taskShedular);
			}
		}
		
		/*
		Should be called by a thread that can only do primary tasks.
		*/
		void startPrimary(TaskShedular& taskShedular, ThreadResources& threadResources, void* context)
		{
			taskShedular.barrier.startCannotLeave(barrier);
			start(taskShedular, threadResources, context);
		}

		/*
		Should be called by a thread that can do primary and background tasks.
		*/
		void startBackground(TaskShedular& taskShedular, ThreadResources& threadResources, void* context)
		{
			taskShedular.barrier.startCanLeave(barrier);
			start(taskShedular, threadResources, context);
		}
	};
private:
	std::unique_ptr<WorkStealingQueue<Task>*[]> mWorkStealingQueuesArray;
	WorkStealingQueue<Task>** mCurrentWorkStealingQueues;
	std::unique_ptr<BackgroundQueue<Task>*[]> mBackgroundQueues;
	unsigned int mThreadCount;
	NextPhaseTask mNextPhaseTask;
	ThreadBarrier barrier;
	std::array<PrimaryTaskFromOtherThreadQueue, numberOfPrimaryQueues> primaryFromOtherThreadQueues;

	template<unsigned int phaseIndex>
	void endUpdate() noexcept
	{
		static_assert(phaseIndex < numberOfPrimaryQueues);
		mCurrentWorkStealingQueues += mThreadCount;
		if constexpr (phaseIndex == (numberOfPrimaryQueues - 1u))
		{
			if (mCurrentWorkStealingQueues == (mWorkStealingQueuesArray.get() + mThreadCount * 4u))
			{
				mCurrentWorkStealingQueues = mWorkStealingQueuesArray.get();
			}
		}
	}
public:
	TaskShedular(unsigned int numberOfThreads, NextPhaseTask nextPhaseTask) :
		primaryFromOtherThreadQueues{ makeArray<numberOfPrimaryQueues>([](std::size_t i)
			{
				return PrimaryTaskFromOtherThreadQueue(i);
			}) },
		mNextPhaseTask(nextPhaseTask),
		barrier(numberOfThreads),
		mWorkStealingQueuesArray(new WorkStealingQueue<Task>*[numberOfThreads * 2u * numberOfPrimaryQueues]),
		mBackgroundQueues(new BackgroundQueue<Task>*[numberOfThreads]),
		mCurrentWorkStealingQueues(mWorkStealingQueuesArray.get()),
		mThreadCount(numberOfThreads)
{}

	unsigned int threadCount() const noexcept
	{
		return mThreadCount;
	}

	unsigned int lockAndGetPrimaryThreadCount() noexcept
	{
		return barrier.lockAndGetThreadCount();
	}

	NextPhaseTask getNextPhaseTask() const noexcept
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

	/*
		Can be called from any thread
	*/
	void pushPrimaryTaskFromOtherThread(std::size_t index, PrimaryTaskFromOtherThreadQueue::Task& task) noexcept
	{
		assert(index < numberOfPrimaryQueues);
		primaryFromOtherThreadQueues[index].push(task);
	}

	void start(ThreadResources& threadResources)
	{
		for(std::size_t i = 0u; i != numberOfPrimaryQueues; ++i)
		{
			primaryFromOtherThreadQueues[i].start(threadResources);
		}
	}

	class StopRequest
	{
		friend class TaskShedular<ThreadResources>;
		using Self = StopRequest;

		class PrimaryFromOtherStopRequest : public PrimaryTaskFromOtherThreadQueue::StopRequest
		{
			using Base = PrimaryTaskFromOtherThreadQueue::StopRequest;
		public:
			Self* stopRequest;

			PrimaryFromOtherStopRequest(void(*callback1)(Base& stopRequest, void* tr), Self* stopRequest1) :
				Base(callback1),
				stopRequest(stopRequest1) {}
		};

		constexpr static std::size_t numberOfComponents = numberOfPrimaryQueues;
		std::atomic<std::size_t> numberOfComponentsStopped = 0u;
		std::array<PrimaryFromOtherStopRequest, numberOfPrimaryQueues> primaryFromOtherStopRequests;
		void(*callback)(StopRequest& stopRequest, void* tr);

		static void componentStopped(PrimaryTaskFromOtherThreadQueue::StopRequest& request, void* tr)
		{
			auto& stopRequest = *static_cast<PrimaryFromOtherStopRequest&>(request).stopRequest;
			if(stopRequest.numberOfComponentsStopped.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
			{
				stopRequest.callback(stopRequest, tr);
			}
		}
	public:
		StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr)) : callback(callback1),
			primaryFromOtherStopRequests{makeArray<numberOfPrimaryQueues>([this]()
				{
					return PrimaryFromOtherStopRequest(componentStopped, this);
				}) }
		{}
	};

	void stop(StopRequest& stopRequest, ThreadResources& threadResources)
	{
		for(std::size_t i = 0u; i != numberOfPrimaryQueues; ++i)
		{
			primaryFromOtherThreadQueues[i].stop(stopRequest.primaryFromOtherStopRequests[i], threadResources);
		}
	}
};
