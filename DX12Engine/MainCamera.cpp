#include "MainCamera.h"
#include "SharedResources.h"
#include "CameraUtil.h"

MainCamera::MainCamera(SharedResources* sharedResources, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
	uint8_t*& constantBufferCpuAddress1, float fieldOfView, const Transform& target) :
	renderTargetViewDescriptorHeap(sharedResources->graphicsEngine.graphicsDevice, []()
{
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.NumDescriptors = frameBufferCount;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	renderTargetViewHeapDesc.NodeMask = 0;
	return renderTargetViewHeapDesc;
}()),
	width(width),
	height(height),
	mImage(sharedResources->window.getBuffer(sharedResources->graphicsEngine.frameIndex)),
	depthSencilView(depthSencilView)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC backBufferSrvDesc;
	backBufferSrvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	backBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	backBufferSrvDesc.Texture2D.MipLevels = 1u;
	backBufferSrvDesc.Texture2D.MostDetailedMip = 0u;
	backBufferSrvDesc.Texture2D.PlaneSlice = 0u;
	backBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0u;
	auto shaderResourceViewCpuDescriptorHandle = sharedResources->graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE backbufferRenderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		sharedResources->graphicsEngine.graphicsDevice->CreateRenderTargetView(sharedResources->window.getBuffer(i), nullptr, backbufferRenderTargetViewHandle);
		backbufferRenderTargetViewHandle.ptr += sharedResources->graphicsEngine.renderTargetViewDescriptorSize;

		backBufferTextures[i] = sharedResources->graphicsEngine.descriptorAllocator.allocate();
		sharedResources->graphicsEngine.graphicsDevice->CreateShaderResourceView(sharedResources->window.getBuffer(i), &backBufferSrvDesc,
			shaderResourceViewCpuDescriptorHandle + backBufferTextures[i] * sharedResources->graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize);
	}
	auto frameIndex = sharedResources->graphicsEngine.frameIndex;
	auto renderTargetView = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetView.ptr += frameIndex * sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
	auto depthStencilView = sharedResources->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	

	constantBufferCpuAddress = reinterpret_cast<CameraConstantBuffer*>(constantBufferCpuAddress1);
	constantBufferCpuAddress1 += bufferSizePS * frameBufferCount;
	constantBufferGpuAddress = constantBufferGpuAddress1;
	constantBufferGpuAddress1 += bufferSizePS * frameBufferCount;

	mLocation.position = target.position;
	mLocation.rotation = target.rotation;

	DirectX::XMMATRIX mViewMatrix = mLocation.toMatrix();

	const float screenAspect = static_cast<float>(width) / static_cast<float>(height);
	mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		auto constantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + i * bufferSizePS);
		constantBuffer->viewProjectionMatrix = mViewMatrix * mProjectionMatrix;
		constantBuffer->cameraPosition = mLocation.position;
		constantBuffer->screenWidth = (float)width;
		constantBuffer->screenHeight = (float)height;
		constantBuffer->backBufferTexture = backBufferTextures[i];
	}

	mFrustum.update(mProjectionMatrix, mViewMatrix, screenNear, screenDepth);
}

void MainCamera::destruct(SharedResources* sharedResources)
{
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		sharedResources->graphicsEngine.descriptorAllocator.deallocate(backBufferTextures[i]);
	}
}

MainCamera::~MainCamera() {}

void MainCamera::update(SharedResources* const sharedResources, const Transform& target)
{
	mLocation.position = target.position;
	mLocation.rotation = target.rotation;

	DirectX::XMMATRIX mViewMatrix = mLocation.toMatrix();

	mImage = sharedResources->window.getBuffer(sharedResources->graphicsEngine.frameIndex);

	const auto constantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + sharedResources->graphicsEngine.frameIndex * bufferSizePS);
	constantBuffer->viewProjectionMatrix = mViewMatrix * mProjectionMatrix;;
	constantBuffer->cameraPosition = mLocation.position;

	mFrustum.update(mProjectionMatrix, mViewMatrix, screenNear, screenDepth);
}

void MainCamera::bind(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	auto frameIndex = sharedResources->graphicsEngine.frameIndex;
	auto renderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewHandle.ptr += frameIndex * sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
	auto constantBufferGPU = constantBufferGpuAddress + bufferSizePS * frameIndex;
	CameraUtil::bind(first, end, CameraUtil::getViewPort(width, height), CameraUtil::getScissorRect(width, height), constantBufferGPU, &renderTargetViewHandle, &depthSencilView);
}

void MainCamera::bindFirstThread(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	bind(sharedResources, first, end);
	auto frameIndex = sharedResources->graphicsEngine.frameIndex;
	auto renderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		frameIndex * sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
	auto commandList = *first;
	constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(renderTargetViewHandle, clearColor, 0u, nullptr);
	commandList->ClearDepthStencilView(depthSencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0u, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE MainCamera::getRenderTargetView(SharedResources* sharedResources)
{
	auto frameIndex = sharedResources->graphicsEngine.frameIndex;
	return renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		frameIndex * sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
}