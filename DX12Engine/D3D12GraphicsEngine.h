#pragma once

#include "D3D12CommandQueue.h"
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12Fence.h"
#include "Win32Event.h"
#include "DXGIFactory.h"
#include "SinglyLinked.h"
#include "LinkedTask.h"
#include "UnorderedMultiProducerSingleConsumerQueue.h"
#include "frameBufferCount.h"
#include <mutex>
#include <cassert>
struct ID3D12GraphicsCommandList;
struct ID3D12CommandList;
class Window;

class D3D12GraphicsEngine
{
	class GpuFrameCompletionQueue
	{
	public:
		using Task = LinkedTask;

		class StopRequest
		{
			friend class GpuFrameCompletionQueue;
			StopRequest** stopRequest;
			void(*callback)(StopRequest& stopRequest, void* tr, void* gr);
		public:
			StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr, void* gr)) : callback(callback1) {}
		};
	private:
		UnorderedMultiProducerSingleConsumerQueue queues[frameBufferCount];
		StopRequest* mStopRequest;

		template<class ThreadResources, class GlobalResources>
		void run(ThreadResources& threadResources, GlobalResources&)
		{
			threadResources.taskShedular.pushPrimaryTask(1u, {this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				GpuFrameCompletionQueue& queue = *static_cast<GpuFrameCompletionQueue*>(requester);
				if(queue.mStopRequest != nullptr)
				{
					queue.mStopRequest->callback(*queue.mStopRequest, &threadResources, &globalResources);
					queue.mStopRequest = nullptr;
					return;
				}
				SinglyLinked* tasks = queue.queues[globalResources.graphicsEngine.frameIndex].popAll();
				while(tasks != nullptr)
				{
					Task& task = *static_cast<Task*>(tasks);
					tasks = tasks->next;
					task.execute(task, &threadResources, &globalResources);
				}
				queue.run(threadResources, globalResources);
			}});
		}
	public:
		/*
		Must be called from primary thread
		*/
		template<class ThreadResources, class GlobalResources>
		void start(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			assert(mStopRequest == nullptr && "Cannot start while not stopped");
			run(threadResources, globalResources);
		}

		/*
		Must be called from primary thread
		*/
		template<class ThreadResources, class GlobalResources>
		void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources&)
		{
			stopRequest.stopRequest = &mStopRequest;
			threadResources.taskShedular.pushPrimaryTask(0u, {&stopRequest, [](void* requester, ThreadResources&, GlobalResources&)
			{
				StopRequest& stopRequest = *static_cast<StopRequest*>(requester);
				*stopRequest.stopRequest = &stopRequest;
			}});
		}

		void push(unsigned long frameIndex, Task& task);
	};
public:
	DXGIAdapter adapter;
	D3D12Device graphicsDevice;
	D3D12CommandQueue directCommandQueue;
private:
	D3D12Fence directFence;
	uint64_t fenceValue;
	Event directFenceEvent;
	GpuFrameCompletionQueue gpuFrameCompletionQueue;
public:
	uint32_t frameIndex;

	uint32_t renderTargetViewDescriptorSize;
	uint32_t cbvAndSrvAndUavDescriptorSize;

	D3D12DescriptorHeap depthStencilDescriptorHeap;
	D3D12Resource depthStencilHeap;
private:
	class DescriptorAllocator
	{
	public:
		constexpr static unsigned int maxDescriptors = 25u;
	private:
		std::mutex descriptorHeapMutex;
		unsigned int freeStack[maxDescriptors];
		unsigned int freeStackEnd;
	public:
		DescriptorAllocator()
		{
			for (auto i = 0u; i < maxDescriptors; ++i)
			{
				freeStack[i] = i;
			}
			freeStackEnd = maxDescriptors;
		}
		unsigned int allocate()
		{
			std::lock_guard<decltype(descriptorHeapMutex)> lock(descriptorHeapMutex);
			--freeStackEnd;
			return freeStack[freeStackEnd];
		}
		void deallocate(unsigned int descriptor)
		{
			std::lock_guard<decltype(descriptorHeapMutex)> lock(descriptorHeapMutex);
			freeStack[freeStackEnd] = descriptor;
			++freeStackEnd;
		}
	};

	struct AdapterAndDeviceAndResourceBindingTier
	{
		IDXGIAdapter3* adapter;
		ID3D12Device* device;
		D3D12_RESOURCE_BINDING_TIER resourceBindingTier;
	};

	D3D12GraphicsEngine(Window& window, DXGIFactory factory, DXGI_ADAPTER_FLAG avoidedAdapterFlags);
	D3D12GraphicsEngine(Window& window, IDXGIFactory5* factory, AdapterAndDeviceAndResourceBindingTier adapterAndDevice);
public:
	D3D12DescriptorHeap mainDescriptorHeap;
	DescriptorAllocator descriptorAllocator;

	D3D12GraphicsEngine(Window& window, bool enableGpuDebugging, DXGI_ADAPTER_FLAG avoidedAdapterFlags = DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_NONE);
	~D3D12GraphicsEngine();

	void endFrame(Window& window, ID3D12CommandList** ppCommandLists, unsigned int numLists);
	void startFrame();
	void waitForPreviousFrame(ID3D12CommandQueue& commandQueue);

	using Task = GpuFrameCompletionQueue::Task;
	void executeWhenGpuFinishesCurrentFrame(Task& task);

	template<class ThreadResources, class GlobalResources>
	void start(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		gpuFrameCompletionQueue.start(threadResources, globalResources);
	}

	using StopRequest = GpuFrameCompletionQueue::StopRequest;
	template<class ThreadResources, class GlobalResources>
	void stop(StopRequest& stopRequest, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		gpuFrameCompletionQueue.stop(stopRequest, threadResources, globalResources);
	}
};
void operator+=(D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::size_t offset);
D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE handle, std::size_t offset);