#include "MainCamera.h"
#include "CameraUtil.h"
#include "Window.h"
#include "GraphicsEngine.h"

MainCamera::MainCamera(Window& window, GraphicsEngine& graphicsEngine, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
	unsigned char*& constantBufferCpuAddress1, float fieldOfView, const Transform& target) :
	renderTargetViewDescriptorHeap(graphicsEngine.graphicsDevice, []()
{
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.NumDescriptors = frameBufferCount;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	renderTargetViewHeapDesc.NodeMask = 0;
	return renderTargetViewHeapDesc;
}()),
	mWidth(width),
	mHeight(height),
	mImage(window.getBuffer(graphicsEngine.frameIndex)),
	depthSencilView(graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart())
{
	auto currentRenderTargetView = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto& renderTargetView : renderTargetViews)
	{
		renderTargetView = currentRenderTargetView;
		currentRenderTargetView += graphicsEngine.renderTargetViewDescriptorSize;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC backBufferSrvDesc;
	backBufferSrvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	backBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	backBufferSrvDesc.Texture2D.MipLevels = 1u;
	backBufferSrvDesc.Texture2D.MostDetailedMip = 0u;
	backBufferSrvDesc.Texture2D.PlaneSlice = 0u;
	backBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0u;
	auto shaderResourceViewCpuDescriptorHandle = graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE backbufferRenderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		graphicsEngine.graphicsDevice->CreateRenderTargetView(window.getBuffer(i), nullptr, backbufferRenderTargetViewHandle);
		backbufferRenderTargetViewHandle.ptr += graphicsEngine.renderTargetViewDescriptorSize;

		backBufferTextures[i] = graphicsEngine.descriptorAllocator.allocate();
		graphicsEngine.graphicsDevice->CreateShaderResourceView(window.getBuffer(i), &backBufferSrvDesc,
			shaderResourceViewCpuDescriptorHandle + backBufferTextures[i] * graphicsEngine.cbvAndSrvAndUavDescriptorSize);
	}
	auto frameIndex = graphicsEngine.frameIndex;
	auto renderTargetView = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetView.ptr += frameIndex * graphicsEngine.renderTargetViewDescriptorSize;
	auto depthStencilView = graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	

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
		constantBuffer->cameraPosition.x = mLocation.position.x();
		constantBuffer->cameraPosition.y = mLocation.position.y();
		constantBuffer->cameraPosition.z = mLocation.position.z();
		constantBuffer->screenWidth = (float)width;
		constantBuffer->screenHeight = (float)height;
		constantBuffer->backBufferTexture = backBufferTextures[i];
	}

	mFrustum.update(mProjectionMatrix, mViewMatrix, screenNear, screenDepth);
}

void MainCamera::destruct(GraphicsEngine& graphicsEngine)
{
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		graphicsEngine.descriptorAllocator.deallocate(backBufferTextures[i]);
	}
}

MainCamera::~MainCamera() {}

void MainCamera::update(const Transform& target)
{
	mLocation.position = target.position;
	mLocation.rotation = target.rotation;
}

void MainCamera::beforeRender(Window& window, GraphicsEngine& graphicsEngine)
{
	DirectX::XMMATRIX mViewMatrix = mLocation.toMatrix();
	mImage = window.getBuffer(graphicsEngine.frameIndex);
	const auto constantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + graphicsEngine.frameIndex * bufferSizePS);
	constantBuffer->viewProjectionMatrix = mViewMatrix * mProjectionMatrix;
	constantBuffer->cameraPosition.x = mLocation.position.x();
	constantBuffer->cameraPosition.y = mLocation.position.y();
	constantBuffer->cameraPosition.z = mLocation.position.z();
	mFrustum.update(mProjectionMatrix, mViewMatrix, screenNear, screenDepth);
}

void MainCamera::bind(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	auto renderTargetViewHandle = renderTargetViews[frameIndex];
	auto constantBufferGPU = constantBufferGpuAddress + bufferSizePS * frameIndex;
	CameraUtil::bind(first, end, CameraUtil::getViewPort(mWidth, mHeight), CameraUtil::getScissorRect(mWidth, mHeight), constantBufferGPU, renderTargetViewHandle, depthSencilView);
}

void MainCamera::bindFirstThread(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	bind(frameIndex, first, end);
	auto renderTargetViewHandle = renderTargetViews[frameIndex];
	auto commandList = *first;
	constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(renderTargetViewHandle, clearColor, 0u, nullptr);
	commandList->ClearDepthStencilView(depthSencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0u, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE MainCamera::getRenderTargetView(uint32_t frameIndex)
{
	return renderTargetViews[frameIndex];
}