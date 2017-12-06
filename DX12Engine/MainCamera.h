#pragma once

#include <DirectXMath.h>
#include <d3d12.h>
#include "Location.h"
#include "frameBufferCount.h"
#include "D3D12DescriptorHeap.h"
class Frustum;
class BaseExecutor;
struct CameraConstantBuffer;
class D3D12GraphicsEngine;
class Window;
class SharedResources;

class MainCamera
{
	MainCamera(const MainCamera&) = delete;

	CameraConstantBuffer* cameraConstantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS cameraConstantBufferGpuAddress;

	Location location;

	D3D12DescriptorHeap renderTargetViewDescriptorHeap;
public:
	MainCamera() {}
	MainCamera(D3D12GraphicsEngine& graphicsEngine, Window& window, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, float fieldOfView);
	~MainCamera();

	void update(BaseExecutor* const executor, const Location& target);
	void makeReflectionMatrix(DirectX::XMMATRIX& reflectionMatrix, float height);
	bool isInView(const Frustum&) { return true; }
	void bind(SharedResources& sharedResources, uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	void bindFirstThread(SharedResources& sharedResources, uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	ID3D12Resource* getImage(BaseExecutor* const executor);
	DirectX::XMFLOAT3& position() { return location.position; }

	DirectX::XMMATRIX viewMatrix;
	DirectX::XMMATRIX projectionMatrix;

	constexpr static float screenDepth = 128.0f * 31.5f;
	constexpr static float screenNear = 0.1f;
};