#include "ReflectionCamera.h"
#include "CameraUtil.h"

ReflectionCamera::ReflectionCamera(D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1, float fieldOfView,
	const Transform& target, GraphicsEngine& graphicsEngine) :
	mWidth(width),
	mHeight(height),
	mImage(graphicsEngine.graphicsDevice, []()
		{
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.VisibleNodeMask = 0u;
			heapProperties.CreationNodeMask = 0u;
			return heapProperties;
		}(),
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			[width, height]()
		{
			D3D12_RESOURCE_DESC resourcesDesc;
			resourcesDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourcesDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resourcesDesc.Width = width;
			resourcesDesc.Height = height;
			resourcesDesc.DepthOrArraySize = 1u;
			resourcesDesc.MipLevels = 1u;
			resourcesDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			resourcesDesc.SampleDesc = { 1u, 0u };
			resourcesDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resourcesDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			return resourcesDesc;
		}(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			[]()
		{
			D3D12_CLEAR_VALUE clearValue;
			clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			clearValue.Color[0] = 0.0f;
			clearValue.Color[1] = 0.0f;
			clearValue.Color[2] = 0.0f;
			clearValue.Color[3] = 1.0f;
			return clearValue;
		}()),
	renderTargetDescriptorHeap(graphicsEngine.graphicsDevice, []()
		{
			D3D12_DESCRIPTOR_HEAP_DESC descriptorheapDesc;
			descriptorheapDesc.NumDescriptors = 1u;
			descriptorheapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //RTVs aren't shader visable
			descriptorheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			descriptorheapDesc.NodeMask = 0u;
			return descriptorheapDesc;
		}()),
	depthSencilView(depthSencilView),
	mProjectionMatrix{ DirectX::XMMatrixPerspectiveFovLH(fieldOfView, /*screenAspect*/static_cast<float>(width) / static_cast<float>(height), screenNear, screenDepth) },
	fieldOfView(fieldOfView),
	shaderResourceViewIndex(graphicsEngine.descriptorAllocator.allocate()),
	mLocation{ target }
{
	constantBufferCpuAddress = reinterpret_cast<CameraConstantBuffer*>(constantBufferCpuAddress1);
	constantBufferCpuAddress1 += bufferSizePS * frameBufferCount;
	constantBufferGpuAddress = constantBufferGpuAddress1;
	constantBufferGpuAddress1 += bufferSizePS * frameBufferCount;

	D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0u;
	renderTargetViewDesc.Texture2D.PlaneSlice = 0u;
	graphicsEngine.graphicsDevice->CreateRenderTargetView(mImage, &renderTargetViewDesc, renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC backBufferSrvDesc;
	backBufferSrvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	backBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	backBufferSrvDesc.Texture2D.MipLevels = 1u;
	backBufferSrvDesc.Texture2D.MostDetailedMip = 0u;
	backBufferSrvDesc.Texture2D.PlaneSlice = 0u;
	backBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	graphicsEngine.graphicsDevice->CreateShaderResourceView(mImage, &backBufferSrvDesc, graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() + shaderResourceViewIndex * graphicsEngine.cbvAndSrvAndUavDescriptorSize);

	for (auto i = 0u; i != frameBufferCount; ++i)
	{
		auto constantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + i * bufferSizePS);
		constantBuffer->screenWidth = (float)width;
		constantBuffer->screenHeight = (float)height;
		constantBuffer->backBufferTexture = shaderResourceViewIndex;
	}

#ifndef NDEBUG
	renderTargetDescriptorHeap->SetName(L"Reflection Render Target texture descriptor heap");
	mImage->SetName(L"Reflection texture");
#endif
}

void ReflectionCamera::destruct(GraphicsEngine& graphicsEngine)
{
	graphicsEngine.descriptorAllocator.deallocate(shaderResourceViewIndex);
}

void ReflectionCamera::resize(unsigned int width, unsigned int height, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView1, GraphicsEngine& graphicsEngine)
{
	mImage = D3D12Resource(graphicsEngine.graphicsDevice, []()
		{
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.VisibleNodeMask = 0u;
			heapProperties.CreationNodeMask = 0u;
			return heapProperties;
		}(),
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			[width, height]()
		{
			D3D12_RESOURCE_DESC resourcesDesc;
			resourcesDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourcesDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resourcesDesc.Width = width;
			resourcesDesc.Height = height;
			resourcesDesc.DepthOrArraySize = 1u;
			resourcesDesc.MipLevels = 1u;
			resourcesDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			resourcesDesc.SampleDesc = { 1u, 0u };
			resourcesDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resourcesDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			return resourcesDesc;
		}(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			[]()
		{
			D3D12_CLEAR_VALUE clearValue;
			clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			clearValue.Color[0] = 0.0f;
			clearValue.Color[1] = 0.0f;
			clearValue.Color[2] = 0.0f;
			clearValue.Color[3] = 1.0f;
			return clearValue;
		}());
	mWidth = width;
	mHeight = height;
	depthSencilView = depthSencilView1;
	float aspectRation = static_cast<float>(width) / static_cast<float>(height);
	mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, aspectRation, screenNear, screenDepth);

	for (auto i = 0u; i != frameBufferCount; ++i)
	{
		auto constantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + i * bufferSizePS);
		constantBuffer->screenWidth = (float)width;
		constantBuffer->screenHeight = (float)height;
	}

	D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0u;
	renderTargetViewDesc.Texture2D.PlaneSlice = 0u;
	graphicsEngine.graphicsDevice->CreateRenderTargetView(mImage, &renderTargetViewDesc, renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC backBufferSrvDesc;
	backBufferSrvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	backBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	backBufferSrvDesc.Texture2D.MipLevels = 1u;
	backBufferSrvDesc.Texture2D.MostDetailedMip = 0u;
	backBufferSrvDesc.Texture2D.PlaneSlice = 0u;
	backBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	graphicsEngine.graphicsDevice->CreateShaderResourceView(mImage, &backBufferSrvDesc, graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() + shaderResourceViewIndex * graphicsEngine.cbvAndSrvAndUavDescriptorSize);

}

void ReflectionCamera::beforeRender(uint32_t frameIndex, const DirectX::XMMATRIX& viewMatrix)
{
	const auto constantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + frameIndex * bufferSizePS);
	constantBuffer->viewProjectionMatrix = viewMatrix * mProjectionMatrix;;
	constantBuffer->cameraPosition.x = mLocation.position.x();
	constantBuffer->cameraPosition.y = mLocation.position.y();
	constantBuffer->cameraPosition.z = mLocation.position.z();

	mFrustum.update(mProjectionMatrix, viewMatrix, screenNear, screenDepth);
}

void ReflectionCamera::bind(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	auto constantBufferGPU = constantBufferGpuAddress + bufferSizePS * frameIndex;
	CameraUtil::bind(first, end, CameraUtil::getViewPort(mWidth, mHeight), CameraUtil::getScissorRect(mWidth, mHeight), constantBufferGPU, getRenderTargetView(), depthSencilView);
}

void ReflectionCamera::bindFirstThread(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	bind(frameIndex, first, end);
	auto commandList = *first;
	constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(getRenderTargetView(), clearColor, 0u, nullptr);
	commandList->ClearDepthStencilView(depthSencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0u, nullptr);
}