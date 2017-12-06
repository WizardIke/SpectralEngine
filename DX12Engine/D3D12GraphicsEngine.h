#pragma once

#include "D3D12CommandQueue.h"
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12DevicePointer.h"
#include "D3D12FencePointer.h"
#include "../Array/Array.h"
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
	DXGIFactory dxgiFactory;
	D3D12Device graphicsDevice;
	D3D12CommandQueuePointer directCommandQueue;

	uint32_t frameIndex;
	Array<D3D12FencePointer, frameBufferCount> directFences;
	Array<uint64_t, frameBufferCount> fenceValues;
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
public:
	D3D12DescriptorHeap mainDescriptorHeap;
	DescriptorAllocator descriptorAllocator;

	D3D12GraphicsEngine(Window& window, bool fullScreen, bool vSync);
	~D3D12GraphicsEngine();

	void present(Window& window, ID3D12CommandList** ppCommandLists, unsigned int numLists);
	void waitForPreviousFrame();
};