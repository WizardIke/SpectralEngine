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
#include "frameBufferCount.h"
#include <mutex>
#include <atomic>
#include <cassert>
struct ID3D12GraphicsCommandList;
struct ID3D12CommandList;
class Window;

class GraphicsEngine
{
	class GpuFrameCompletionQueue
	{
	public:
		using Task = LinkedTask;
	private:
#pragma warning(push)
#pragma warning(disable:4324) //warns about padding due to over alignment

		/*
		We can use std::memory_order_relaxed because items are added to the queue in update2 and removed in update1
		*/
		class EventQueue
		{
#if __cplusplus >= 201703L
			static constexpr std::size_t hardwareDestructiveInterferenceSize
				= std::hardware_destructive_interference_size;
#else
			static constexpr std::size_t hardwareDestructiveInterferenceSize = 64u;
#endif
			SinglyLinked head{nullptr};
			alignas(hardwareDestructiveInterferenceSize) std::atomic<SinglyLinked*> tail = &head;
		public:
			void push(SinglyLinked* value) noexcept;

			/*
			 * Returns nullptr if the queue is empty.
			 * Must not be called concurrently with push
			 */
			SinglyLinked* popAll() noexcept;
		};

#pragma warning(pop)

		EventQueue queues[frameBufferCount];
	public:
		/*
		Must be called from startFrame
		*/
		void update(unsigned long frameIndex, void* tr, void* gr)
		{
			SinglyLinked* tasks = queues[frameIndex].popAll();
			while(tasks != nullptr)
			{
				Task& task = *static_cast<Task*>(tasks);
				tasks = tasks->next;
				task.execute(task, tr, gr);
			}
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

	GraphicsEngine(Window& window, DXGIFactory factory, DXGI_ADAPTER_FLAG avoidedAdapterFlags);
	GraphicsEngine(Window& window, IDXGIFactory5* factory, AdapterAndDeviceAndResourceBindingTier adapterAndDevice);
public:
	D3D12DescriptorHeap mainDescriptorHeap;
	DescriptorAllocator descriptorAllocator;

	GraphicsEngine(Window& window, bool enableGpuDebugging, DXGI_ADAPTER_FLAG avoidedAdapterFlags = DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_NONE);
	~GraphicsEngine();

	void endFrame(Window& window, ID3D12CommandList** ppCommandLists, unsigned int numLists);
	void startFrame(void* tr, void* gr);
	void waitForPreviousFrame(ID3D12CommandQueue& commandQueue);

	using Task = GpuFrameCompletionQueue::Task;
	void executeWhenGpuFinishesCurrentFrame(Task& task);
};
void operator+=(D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::size_t offset);
D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE handle, std::size_t offset);