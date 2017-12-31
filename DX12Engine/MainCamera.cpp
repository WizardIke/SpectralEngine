#include "MainCamera.h"
#include "SharedResources.h"

MainCamera::MainCamera(SharedResources* sharedResources, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
	uint8_t*& constantBufferCpuAddress1, float fieldOfView, const Location& target) :
	Camera(),
	renderTargetViewDescriptorHeap(sharedResources->graphicsEngine.graphicsDevice, []()
{
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.NumDescriptors = frameBufferCount;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	renderTargetViewHeapDesc.NodeMask = 0;
	return renderTargetViewHeapDesc;
}())
{
	D3D12_CPU_DESCRIPTOR_HANDLE backbufferRenderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		sharedResources->graphicsEngine.graphicsDevice->CreateRenderTargetView(sharedResources->window.getBuffer(i), nullptr, backbufferRenderTargetViewHandle);
		backbufferRenderTargetViewHandle.ptr += sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
	}
	auto frameIndex = sharedResources->graphicsEngine.frameIndex;
	auto renderTargetView = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetView.ptr += frameIndex * sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
	auto depthStencilView = sharedResources->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	new(static_cast<Camera*>(this)) Camera(sharedResources, sharedResources->window.getBuffer(frameIndex), renderTargetView, depthStencilView, width, height, 
		constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView, target);
}

MainCamera::~MainCamera() {}

void MainCamera::update(SharedResources* const sharedResources, const Location& target)
{
	mLocation.position = target.position;
	mLocation.rotation = target.rotation;

	DirectX::XMMATRIX mViewMatrix = locationToMatrix(mLocation);;

	image = sharedResources->window.getBuffer(sharedResources->graphicsEngine.frameIndex);
	auto renderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewHandle.ptr += sharedResources->graphicsEngine.frameIndex * sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
	renderTargetView = renderTargetViewHandle;

	Camera::update(sharedResources, mViewMatrix);
}