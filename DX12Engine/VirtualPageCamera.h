#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "Transform.h"
#include "frameBufferCount.h"
#include "Frustum.h"
#include "Shaders/VtFeedbackCameraMaterial.h"
#include <new>
class BaseExecutor;
class SharedResources;

class VirtualPageCamera
{
	constexpr static unsigned int bufferSizePS = (sizeof(VtFeedbackCameraMaterial) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u)
		& ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);

	VtFeedbackCameraMaterial* constantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpuAddress;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView;
	Transform* mTransform;
	ID3D12Resource* mImage;
	unsigned int mWidth, mHeight;
	DirectX::XMMATRIX mProjectionMatrix;
public:
	constexpr static float screenDepth = 128.0f * 31.5f;
	constexpr static float screenNear = 0.1f;

	VirtualPageCamera() {}
	VirtualPageCamera(SharedResources* sharedResources, ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
		unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
		Transform& target);
	void init(SharedResources* sharedResources, ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
		unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
		Transform& target)
	{
		this->~VirtualPageCamera();
		new(this) VirtualPageCamera(sharedResources, image, renderTargetView, depthSencilView, width, height, constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView, target);
	}
	~VirtualPageCamera();

	void update(SharedResources* sharedResources, float mipBias);
	bool isInView(SharedResources& sharedResources) const { return true; }
	void bind(SharedResources& sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	void bindFirstThread(SharedResources& sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView() { return renderTargetView; }
	ID3D12Resource* getImage() { return mImage; };
	const ID3D12Resource* getImage() const { return mImage; }
	DirectX::XMMATRIX& projectionMatrix() { return mProjectionMatrix; }
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView(SharedResources* sharedResources) { return renderTargetView; }
	unsigned int width() { return mWidth; }
	unsigned int height() { return mHeight; }
};