#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "Transform.h"
#include "frameBufferCount.h"
#include "Frustum.h"
#include "Shaders/CameraConstantBuffer.h"
class BaseExecutor;
class SharedResources;

class ReflectionCamera
{
	constexpr static unsigned int bufferSizePS = (sizeof(CameraConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u)
		& ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);

	CameraConstantBuffer* constantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpuAddress;
	Transform mLocation;
	Frustum mFrustum;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView;
	ID3D12Resource* mImage;
	unsigned int width, height;
	DirectX::XMMATRIX mProjectionMatrix;

	ReflectionCamera(const ReflectionCamera&) = delete;
public:
	constexpr static float screenDepth = 128.0f * 31.5f;
	constexpr static float screenNear = 0.1f;

	ReflectionCamera() {}
	ReflectionCamera(SharedResources* sharedResources, ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
		unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
		const Transform& target, uint32_t* backBufferTextures);
	~ReflectionCamera();

	void update(SharedResources* sharedResources, const DirectX::XMMATRIX& mViewMatrix);
	bool isInView(const BaseExecutor*) { return true; }
	void bind(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	void bindFirstThread(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	ID3D12Resource* getImage() { return mImage; };
	const ID3D12Resource* getImage() const { return mImage;}
	DirectX::XMFLOAT3& position() { return mLocation.position; }
	DirectX::XMMATRIX& projectionMatrix() { return mProjectionMatrix; }
	Transform& transform() { return mLocation; }
	const Frustum& frustum() const { return mFrustum; }
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView(SharedResources* sharedResources) { return renderTargetView; }
};