#include "Assets.h"
#include "TextureNames.h"

static void loadingResourceCallback(void* data, BaseExecutor* exe, SharedResources& sharedResources, unsigned int textureID)
{
	auto& assets = reinterpret_cast<Assets&>(sharedResources);
	assets.arial.setDiffuseTexture(textureID, assets.constantBuffersCpuAddress, assets.sharedConstantBuffer->GetGPUVirtualAddress());
	assets.start(exe);
}

Assets::Assets() :
	SharedResources(false, true, false, std::thread::hardware_concurrency(), SharedResources::windowCallback<Assets>),
	mainExecutor(*this),
	inputHandler(window, { PlayerPosition::mouseMoved, &playerPosition }),
	rootSignatures(graphicsEngine.graphicsDevice),
	pipelineStateObjects(graphicsEngine.graphicsDevice, rootSignatures),
	virtualTextureManager(graphicsEngine),
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
	ambientMusic(&mainExecutor, *this),
	areas(&mainExecutor, *this),

	warpTexture(graphicsEngine.graphicsDevice, []()
{
	D3D12_HEAP_PROPERTIES properties;
	properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	properties.VisibleNodeMask = 0u;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.CreationNodeMask = 0u;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	return properties;
}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, window.getBuffer(0u)->GetDesc(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr),

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
		new(&element) BackgroundExecutor(*this);
	}),
	primaryExecutors(ArraySize(maxPrimaryThreads), [this](size_t i, PrimaryExecutor& element)
	{
		new(&element) PrimaryExecutor(*this);
	})
{
	D3D12_RANGE readRange{ 0u, 0u };
	HRESULT hr = sharedConstantBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&constantBuffersCpuAddress));
	if (FAILED(hr)) throw HresultException(hr);
	uint8_t* cpuConstantBuffer = constantBuffersCpuAddress;
	auto constantBuffersGpuAddress = sharedConstantBuffer->GetGPUVirtualAddress();

	new(&mainCamera) MainCamera(this, window.width(), window.height(), constantBuffersGpuAddress, cpuConstantBuffer, 0.25f * 3.141f, playerPosition.location);
	new(&renderPass) RenderPass1(*this, mainCamera, constantBuffersGpuAddress, cpuConstantBuffer, 0.25f * 3.141f);
	new(&mUserInterface) UserInterface(*this, constantBuffersGpuAddress, cpuConstantBuffer);
	new(&arial) Font(constantBuffersGpuAddress, cpuConstantBuffer, L"Arial.fnt", TextureNames::Arial, &mainExecutor, *this, this, loadingResourceCallback);

	renderPass.colorSubPass().addCamera(*this, renderPass, &mainCamera);


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1u;
	srvDesc.Texture2D.MostDetailedMip = 0u;
	srvDesc.Texture2D.PlaneSlice = 0u;
	srvDesc.Texture2D.ResourceMinLODClamp = 0u;
	warpTextureDescriptorIndex = graphicsEngine.descriptorAllocator.allocate();
	graphicsEngine.graphicsDevice->CreateShaderResourceView(warpTexture, &srvDesc, graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize * warpTextureDescriptorIndex);

	D3D12_RENDER_TARGET_VIEW_DESC warpTextureRtvDesc;
	warpTextureRtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	warpTextureRtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
	warpTextureRtvDesc.Texture2D.MipSlice = 0u;
	warpTextureRtvDesc.Texture2D.PlaneSlice = 0u;
	graphicsEngine.graphicsDevice->CreateRenderTargetView(warpTexture, &warpTextureRtvDesc, warpTextureCpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

#ifndef NDEBUG
	warpTexture->SetName(L"Warp texture");
	warpTextureCpuDescriptorHeap->SetName(L"Warp texture descriptor heap");
#endif

	checkForWindowsMessages();
	playerPosition.update(&mainExecutor, *this);
	mainCamera.update(this, playerPosition.location);

	{
		std::lock_guard<decltype(syncMutex)> lock(syncMutex);
		nextPhaseJob = Executor::initialize;
	}

	for (auto& executor : backgroundExecutors)
	{
		executor.run(*this);
	}
	for (auto& executor : primaryExecutors)
	{
		executor.run(*this);
	}
	auto& us = userInterface();
	mainExecutor.run(*this);
}

Assets::~Assets()
{
	graphicsEngine.descriptorAllocator.deallocate(warpTextureDescriptorIndex);
	mainCamera.destruct(this);
	userInterface().~UserInterface();
}

void Assets::update(BaseExecutor* const executor)
{
	checkForWindowsMessages();
	timer.update();
	playerPosition.update(executor, *this);
	mainCamera.update(this, playerPosition.location);
	(*renderPass.virtualTextureFeedbackSubPass().cameras().begin())->update(this, virtualTextureManager.pageProvider.mipBias);
	//soundEngine.SetListenerPosition(playerPosition.location.position, DS3D_IMMEDIATE);
}

void Assets::start(BaseExecutor* executor)
{
	timer.start();
	userInterface().start(&mainExecutor, *this);
	//soundEngine.SetListenerPosition(playerPosition.location.position, DS3D_IMMEDIATE);
}