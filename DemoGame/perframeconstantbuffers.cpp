#include "perframeconstantbuffers.h"
#include <HresultException.h>
#include <BaseExecutor.h>
#include <SharedResources.h>
#include <d3d12.h>
#include <D3D12GraphicsEngine.h>

PerFramConstantBuffers::PerFramConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress)
{
	cameraConstantBufferCpuAddress = reinterpret_cast<CameraConstantBuffer*>(constantBufferCpuAddress);
	constantBufferCpuAddress += constantBufferPerObjectAlignedSize * D3D12GraphicsEngine::frameBufferCount;
	cameraConstantBufferGpuAddress = constantBufferGpuAddress;
	constantBufferGpuAddress += constantBufferPerObjectAlignedSize * D3D12GraphicsEngine::frameBufferCount;
}

PerFramConstantBuffers::~PerFramConstantBuffers() {}

void PerFramConstantBuffers::update(BaseExecutor* const executor)
{
	auto sharedResources = executor->sharedResources;
	CameraConstantBuffer* cameraConstantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(cameraConstantBufferCpuAddress) + executor->frameIndex * constantBufferPerObjectAlignedSize);
	cameraConstantBuffer->viewProjectionMatrix = sharedResources->mainCamera.viewMatrix * sharedResources->graphicsEngine.window.projectionMatrix;;
	cameraConstantBuffer->cameraPosition = sharedResources->mainCamera.location.position;
}

void PerFramConstantBuffers::bind(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	commandList->SetGraphicsRootConstantBufferView(0u, cameraConstantBufferGpuAddress + constantBufferPerObjectAlignedSize * frameIndex);
}