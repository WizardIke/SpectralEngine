#include "Assets.h"
#include "TextureNames.h"

Assets::Assets() :
	SharedResources(&mainExecutor, false, false),
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
	new(&mainCamera) MainCamera(graphicsEngine, window, constantBuffersGpuAddress, constantBuffersCpuAddress, 0.25f * 3.141f);

	renderPass.colorSubPass().addCamera(&mainExecutor, &mainCamera);


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

void Assets::update(BaseExecutor* const executor)
{
	checkForWindowsMessages(executor);
	timer.update();
	playerPosition.update(executor);
	mainCamera.update(executor, playerPosition.location);
	mainFrustum.update(mainCamera.projectionMatrix, mainCamera.viewMatrix, mainCamera.screenNear, mainCamera.screenDepth);
	//soundEngine.SetListenerPosition(playerPosition.location.position, DS3D_IMMEDIATE);
}

void Assets::start(BaseExecutor* executor)
{
	timer.start();
	checkForWindowsMessages(executor);
	playerPosition.update(executor);
	mainCamera.update(executor, playerPosition.location);
	mainFrustum.update(mainCamera.projectionMatrix, mainCamera.viewMatrix, mainCamera.screenNear, mainCamera.screenDepth);
	//soundEngine.SetListenerPosition(playerPosition.location.position, DS3D_IMMEDIATE);

	{
		std::lock_guard<decltype(syncMutex)> lock(syncMutex);
		nextPhaseJob = Executor::initialize;
	}
}