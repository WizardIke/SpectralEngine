#include "GraphicsEngine.h"
#include "HresultException.h"
#include <d3d12.h>
#include "DXGIAdapter.h"
#include "Window.h"
#include "GraphicsAdapterNotFound.h"

GraphicsEngine::GraphicsEngine(Window& window, bool enableGpuDebugging,
#ifndef NDEBUG
	bool& isWarp,
#endif
	DXGI_ADAPTER_FLAG avoidedAdapterFlags) : GraphicsEngine(window, DXGIFactory(enableGpuDebugging),
#ifndef NDEBUG
		isWarp,
#endif
		avoidedAdapterFlags)
{}

GraphicsEngine::GraphicsEngine(Window& window, DXGIFactory factory,
#ifndef NDEBUG
	bool& isWarp,
#endif
	DXGI_ADAPTER_FLAG avoidedAdapterFlags) : GraphicsEngine(window, factory, [&factory, &window,
#ifndef NDEBUG
		&isWarp,
#endif
		avoidedAdapterFlags]()
{
	AdapterAndDeviceAndResourceBindingTier adapterAndDeviceAndResourceBindingTier = {};

	IDXGIAdapter3* adapter3;
	IDXGIAdapter1* adapter1;
	DXGI_ADAPTER_DESC1 desc;
	bool found = false;
	for(auto adapterIndex = 0u; factory->EnumAdapters1(adapterIndex, &adapter1) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) // we'll start looking for directx 12 compatible graphics devices starting at index 0
	{
		adapter1->GetDesc1(&desc);
		auto result = adapter1->QueryInterface(IID_PPV_ARGS(&adapter3));
		adapter1->Release();
		if(result == S_OK)
		{
			if((desc.Flags & avoidedAdapterFlags) == 0u)
			{
				ID3D12Device* device;
				HRESULT hr = D3D12CreateDevice(adapter1, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
				if(FAILED(hr))
				{
					adapter3->Release();
					continue;
				}

				D3D12_FEATURE_DATA_D3D12_OPTIONS featureData;
				device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureData, sizeof(featureData));
				if(featureData.TiledResourcesTier < D3D12_TILED_RESOURCES_TIER::D3D12_TILED_RESOURCES_TIER_2)
				{
					adapter3->Release();
					device->Release();
					continue;
				}

				adapterAndDeviceAndResourceBindingTier.adapter = adapter3;
				adapterAndDeviceAndResourceBindingTier.device = device;
				adapterAndDeviceAndResourceBindingTier.resourceBindingTier = featureData.ResourceBindingTier;
				found = true;
#ifndef NDEBUG
				isWarp = ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0u);
#endif
				break;
			}
			adapter3->Release();
		}
	}
	
	if(!found)
	{
		MessageBoxW(window.native_handle(), L"No DirectX 12 (D3D_FEATURE_LEVEL_11_0 or later with D3D12_TILED_RESOURCES_TIER_2 support) GPU found.", L"Error", MB_OK);
		throw GraphicsAdapterNotFound();
	}
	
	return adapterAndDeviceAndResourceBindingTier;
}()) {}

GraphicsEngine::GraphicsEngine(Window& window, IDXGIFactory5* factory, AdapterAndDeviceAndResourceBindingTier adapterAndDeviceAndResourceBindingTier) :
	adapter(adapterAndDeviceAndResourceBindingTier.adapter),
	graphicsDevice(adapterAndDeviceAndResourceBindingTier.device),
	directCommandQueue(graphicsDevice, []()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0u;
	return commandQueueDesc;
}()),
directFenceEvent(nullptr, FALSE, FALSE, nullptr),
directFence(graphicsDevice, frameBufferCount, D3D12_FENCE_FLAG_NONE),
fenceValue(frameBufferCount + 1u),
renderTargetViewDescriptorSize(graphicsDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
cbvAndSrvAndUavDescriptorSize(graphicsDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
depthStencilDescriptorHeap(graphicsDevice, []()
{
	D3D12_DESCRIPTOR_HEAP_DESC depthStencilViewDescriptorHeapDesc;
	depthStencilViewDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	depthStencilViewDescriptorHeapDesc.NumDescriptors = 1;
	depthStencilViewDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	depthStencilViewDescriptorHeapDesc.NodeMask = 0;
	return depthStencilViewDescriptorHeapDesc;
}()),
depthStencilHeap(graphicsDevice, []()
{
	D3D12_HEAP_PROPERTIES depthSencilHeapProperties;
	depthSencilHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthSencilHeapProperties.CreationNodeMask = 1u;
	depthSencilHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	depthSencilHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthSencilHeapProperties.VisibleNodeMask = 1u;
	return depthSencilHeapProperties;
}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [&]()
{
	D3D12_RESOURCE_DESC depthSencilHeapResourcesDesc;
	depthSencilHeapResourcesDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthSencilHeapResourcesDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	depthSencilHeapResourcesDesc.Width = window.width();
	depthSencilHeapResourcesDesc.Height = window.height();
	depthSencilHeapResourcesDesc.DepthOrArraySize = 1u;
	depthSencilHeapResourcesDesc.MipLevels = 1u;
	depthSencilHeapResourcesDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthSencilHeapResourcesDesc.SampleDesc.Count = 1u;
	depthSencilHeapResourcesDesc.SampleDesc.Quality = 0u;
	depthSencilHeapResourcesDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthSencilHeapResourcesDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	return depthSencilHeapResourcesDesc;
}(), D3D12_RESOURCE_STATE_DEPTH_WRITE, []()
{
	D3D12_CLEAR_VALUE depthOptimizedClearValue;
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;
	return depthOptimizedClearValue;
}())
{
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
	depthStencilDesc.Texture2D.MipSlice = 0;

	graphicsDevice->CreateDepthStencilView(depthStencilHeap, &depthStencilDesc, depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = adapterAndDeviceAndResourceBindingTier.resourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1 ? 128u : DescriptorAllocator::maxDescriptors;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0u;

	new(&mainDescriptorHeap) D3D12DescriptorHeap(graphicsDevice, heapDesc);

#ifndef NDEBUG
	depthStencilDescriptorHeap->SetName(L"Depth/Stencil Descriptor Heap");
	depthStencilHeap->SetName(L"Depth/Stencil Resource Heap");
	directCommandQueue->SetName(L"Direct command queue");
	graphicsDevice->SetName(L"Graphics device");
	mainDescriptorHeap->SetName(L"Main Descriptor Heap");
#endif // _DEBUG


	if(adapterAndDeviceAndResourceBindingTier.resourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc;
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		nullSrvDesc.Texture2D.MipLevels = 1;
		nullSrvDesc.Texture2D.MostDetailedMip = 0;
		nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		nullSrvDesc.Texture2D.PlaneSlice = 0;

		auto start = mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE end;
		end.ptr = start.ptr + 128u * cbvAndSrvAndUavDescriptorSize;
		for(; start.ptr != end.ptr; start.ptr += cbvAndSrvAndUavDescriptorSize)
		{
			graphicsDevice->CreateShaderResourceView(nullptr, &nullSrvDesc, start);
		}
	}

	window.createSwapChain(*this, factory);
	frameIndex = window.getCurrentBackBufferIndex();
	window.setForgroundAndShow();
}

GraphicsEngine::~GraphicsEngine() {}

void GraphicsEngine::endFrame(Window& window, ID3D12CommandList** const commandLists, const unsigned int numLists)
{
	directCommandQueue->ExecuteCommandLists(numLists, commandLists);
	auto hr = directCommandQueue->Signal(directFence, fenceValue);
	if(FAILED(hr)) throw HresultException(hr);

	++fenceValue;

	window.present();
	frameIndex = window.getCurrentBackBufferIndex();
}

void GraphicsEngine::startFrame(void* tr)
{
	const uint64_t fenceValueToWaitFor = fenceValue - frameBufferCount;
	if (directFence->GetCompletedValue() < fenceValueToWaitFor)
	{
		HRESULT hr = directFence->SetEventOnCompletion(fenceValueToWaitFor, directFenceEvent);
		if (FAILED(hr)) throw HresultException(hr);
		WaitForSingleObject(directFenceEvent, INFINITE);
	}

	gpuFrameCompletionQueue.update(frameIndex, tr);
}

void GraphicsEngine::waitForPreviousFrame(ID3D12CommandQueue& commandQueue)
{
	commandQueue.Wait(directFence, fenceValue - 1u);
}

void GraphicsEngine::executeWhenGpuFinishesCurrentFrame(Task& task)
{
	gpuFrameCompletionQueue.push(frameIndex, task);
}

void operator+=(D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::size_t offset)
{
	handle.ptr += offset;
}

D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE handle, std::size_t offset)
{
	D3D12_CPU_DESCRIPTOR_HANDLE retval;
	retval.ptr = handle.ptr + offset;
	return retval;
}

void GraphicsEngine::GpuFrameCompletionQueue::push(unsigned long frameIndex, Task& task)
{
	assert(frameIndex < frameBufferCount);
	queues[frameIndex].push(&task);
}

void GraphicsEngine::GpuFrameCompletionQueue::EventQueue::push(SinglyLinked* value) noexcept
{
	SinglyLinked* oldTail = tail.exchange(value, std::memory_order_relaxed);
	oldTail->next = value;
}

SinglyLinked* GraphicsEngine::GpuFrameCompletionQueue::EventQueue::popAll() noexcept
{
	SinglyLinked* oldTail = tail.load(std::memory_order_relaxed);
	oldTail->next = nullptr;
	tail.store(&head, std::memory_order_relaxed);
	SinglyLinked* items = head.next;
	head.next = nullptr;
	return items;
}

void GraphicsEngine::stop()
{
	auto hr = directCommandQueue->Signal(directFence, fenceValue);
	if (FAILED(hr)) throw HresultException(hr);

	if (directFence->GetCompletedValue() < fenceValue)
	{
		hr = directFence->SetEventOnCompletion(fenceValue, directFenceEvent);
		if (FAILED(hr)) throw HresultException(hr);
		WaitForSingleObject(directFenceEvent, INFINITE);
	}
}

HANDLE GraphicsEngine::getFrameEvent()
{
	return directFenceEvent;
}