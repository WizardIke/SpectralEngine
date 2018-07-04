#pragma once

#include "D3D12CommandQueue.h"
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12Fence.h"
#include "Array.h"
#include "Win32Event.h"
#include "DXGIFactory.h"
#include <mutex>
#include "frameBufferCount.h"
struct ID3D12GraphicsCommandList;
struct ID3D12CommandList;
class Window;

class D3D12GraphicsEngine
{
public:
	DXGIAdapter adapter;
	D3D12Device graphicsDevice;
	D3D12CommandQueue directCommandQueue;

	uint32_t frameIndex;
	Array<D3D12FencePointer, frameBufferCount> directFences;
	uint64_t fenceValues[frameBufferCount];
	Event directFenceEvent;

	uint32_t renderTargetViewDescriptorSize;
	uint32_t constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;

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

	D3D12GraphicsEngine(Window& window, DXGIFactory factory);
public:
	D3D12DescriptorHeap mainDescriptorHeap;
	DescriptorAllocator descriptorAllocator;

	D3D12GraphicsEngine(Window& window, bool enableGpuDebugging);
	~D3D12GraphicsEngine();

	void present(Window& window, ID3D12CommandList** ppCommandLists, unsigned int numLists);
	void waitForPreviousFrame();
};
void operator+=(D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::size_t offset);
D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE handle, size_t offset);