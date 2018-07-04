#include "D3D12GraphicsEngine.h"
#include "HresultException.h"
#include <d3d12.h>
#include "DXGIAdapter.h"
#include "Window.h"


D3D12GraphicsEngine::D3D12GraphicsEngine(Window& window, bool enableGpuDebugging) : D3D12GraphicsEngine(window, DXGIFactory(enableGpuDebugging))
{}

D3D12GraphicsEngine::D3D12GraphicsEngine(Window& window, DXGIFactory factory) :
	adapter(factory, window.windowHandle, D3D_FEATURE_LEVEL_11_0),
	graphicsDevice(adapter, D3D_FEATURE_LEVEL_11_0),

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
	directFences([&](size_t i, D3D12FencePointer& element)
{
	new(&element) D3D12FencePointer(graphicsDevice, 0u, D3D12_FENCE_FLAG_NONE);
}),
	renderTargetViewDescriptorSize(graphicsDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
	constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize(graphicsDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
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
}(), D3D12_RESOURCE_STATE_DEPTH_WRITE, &[]()
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

	D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
	graphicsDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1 ? 128u : DescriptorAllocator::maxDescriptors;
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


	if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
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
		end.ptr = start.ptr + 128u * constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;
		for (; start.ptr != end.ptr; start.ptr += constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize)
		{
			graphicsDevice->CreateShaderResourceView(nullptr, &nullSrvDesc, start);
		}
	}

	for (auto& fenceValue : fenceValues)
	{
		fenceValue = 0u;
	}

	window.createSwapChain(*this, factory);
	frameIndex = window.getCurrentBackBufferIndex();
	window.setForgroundAndShow();
}

D3D12GraphicsEngine::~D3D12GraphicsEngine() {}

void D3D12GraphicsEngine::present(Window& window, ID3D12CommandList** const commandLists, const unsigned int numLists)
{
	directCommandQueue->ExecuteCommandLists(numLists, commandLists);
	++(fenceValues[frameIndex]);

	auto hr = directCommandQueue->Signal(directFences[frameIndex], fenceValues[frameIndex]);
	if (FAILED(hr)) throw HresultException(hr);

	window.present();
	frameIndex = window.getCurrentBackBufferIndex();
}

void D3D12GraphicsEngine::waitForPreviousFrame()
{
	if (directFences[frameIndex]->GetCompletedValue() < fenceValues[frameIndex])
	{
		HRESULT hr = directFences[frameIndex]->SetEventOnCompletion(fenceValues[frameIndex], directFenceEvent);
		if (FAILED(hr)) throw HresultException(hr);
		WaitForSingleObject(directFenceEvent, INFINITE);
	}
}

void operator+=(D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::size_t offset)
{
	handle.ptr = handle.ptr + offset;
}

D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE handle, std::size_t offset)
{
	D3D12_CPU_DESCRIPTOR_HANDLE retval;
	retval.ptr = handle.ptr + offset;
	return retval;
}