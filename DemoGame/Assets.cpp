#include "Assets.h"
#include "TextureNames.h"

Assets::Assets() :
	SharedResources(&mainExecutor, false, false, std::thread::hardware_concurrency() + 1u),
	mainExecutor(this),
	rootSignatures(graphicsEngine.graphicsDevice),
	pipelineStateObjects(graphicsEngine.graphicsDevice, rootSignatures),
	sharedConstantBuffer(graphicsEngine.graphicsDevice, []()
{
	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 0u;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.VisibleNodeMask = 0u;
	heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	return heapProperties;
}(), D3D12_HEAP_FLAG_NONE, []()
{
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	resourceDesc.Height = 1u;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	resourceDesc.Width = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // size of the resource heap. Must be a multiple of 64KB for constant buffers
	return resourceDesc;
}(), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr),
	userInterface(&mainExecutor),
	ambientMusic(&mainExecutor),
	renderPass(&mainExecutor),
	areas(&mainExecutor),

	warpTexture(graphicsEngine.graphicsDevice, []()
{
	D3D12_HEAP_PROPERTIES properties;
	properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	properties.VisibleNodeMask = 0u;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.CreationNodeMask = 0u;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	return properties;
}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_DENY_BUFFERS, window.getBuffer(0u)->GetDesc(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr),

	warpTextureCpuDescriptorHeap(graphicsEngine.graphicsDevice, []()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	return desc;
}()),

	backgroundExecutors(ArraySize(maxBackgroundThreads), [this](size_t i, BackgroundExecutor& element)
	{
		new(&element) BackgroundExecutor(this);
	}),
	primaryExecutors(ArraySize(maxPrimaryThreads), [this](size_t i, PrimaryExecutor& element)
	{
		new(&element) PrimaryExecutor(this);
	})
{

	uint8_t* constantBuffersCpuAddress;
	D3D12_RANGE readRange{ 0u, 0u };
	HRESULT hr = sharedConstantBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&constantBuffersCpuAddress));
	if (FAILED(hr)) throw HresultException(hr);
	auto constantBuffersGpuAddress = sharedConstantBuffer->GetGPUVirtualAddress();

	new(&arial) Font(constantBuffersGpuAddress, constantBuffersCpuAddress, L"Arial.fnt", TextureNames::Arial, &mainExecutor);
	new(&mainCamera) MainCamera(this, window.width(), window.height(), constantBuffersGpuAddress, constantBuffersCpuAddress, 0.5f * 3.141f, playerPosition.location);

	renderPass.colorSubPass().addCamera(&mainExecutor, renderPass, &mainCamera);
	warpTextureDescriptorIndex = graphicsEngine.descriptorAllocator.allocate();

	start(&mainExecutor);
	userInterface.start(&mainExecutor);

	for (auto& executor : backgroundExecutors)
	{
		executor.run();
	}
	for (auto& executor : primaryExecutors)
	{
		executor.run();
	}

	mainExecutor.run();
}

Assets::~Assets()
{
	graphicsEngine.descriptorAllocator.deallocate(warpTextureDescriptorIndex);
}

void Assets::update(BaseExecutor* const executor)
{
	checkForWindowsMessages(executor);
	timer.update();
	playerPosition.update(executor);
	mainCamera.update(this, playerPosition.location);
	//soundEngine.SetListenerPosition(playerPosition.location.position, DS3D_IMMEDIATE);
}

void Assets::start(BaseExecutor* executor)
{
	timer.start();
	checkForWindowsMessages(executor);
	playerPosition.update(executor);
	mainCamera.update(this, playerPosition.location);
	//soundEngine.SetListenerPosition(playerPosition.location.position, DS3D_IMMEDIATE);

	{
		std::lock_guard<decltype(syncMutex)> lock(syncMutex);
		nextPhaseJob = Executor::initialize;
	}
}