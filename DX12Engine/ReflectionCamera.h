#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "Transform.h"
#include "Vector3.h"
#include "frameBufferCount.h"
#include "Frustum.h"
#include "Shaders/CameraConstantBuffer.h"
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include <cstddef>
#include "GraphicsEngine.h"

class ReflectionCamera
{
	constexpr static std::size_t bufferSizePS = (sizeof(CameraConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u)
		& ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);

	CameraConstantBuffer* constantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpuAddress;
	Transform mLocation;
	Frustum mFrustum;
	D3D12DescriptorHeap renderTargetDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView;
	D3D12Resource mImage;
	float fieldOfView;
	unsigned int shaderResourceViewIndex;
	unsigned int mWidth, mHeight;
	DirectX::XMMATRIX mProjectionMatrix;
public:
	constexpr static float screenDepth = 128.0f * 31.5f;
	constexpr static float screenNear = 0.1f;
	constexpr static std::size_t totalConstantBufferRequired = bufferSizePS * frameBufferCount;

	ReflectionCamera(D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1, float fieldOfView,
		const Transform& target, GraphicsEngine& graphicsEngine);
	ReflectionCamera(ReflectionCamera&&) = default;
	~ReflectionCamera() = default;

	ReflectionCamera& operator=(ReflectionCamera&&) = default;

	void destruct(GraphicsEngine& graphicsEngine);

	void resize(unsigned int width, unsigned int height, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView, GraphicsEngine& graphicsEngine);

	void beforeRender(uint32_t frameIndex, const DirectX::XMMATRIX& mViewMatrix);
	static constexpr bool isInView() { return true; }
	void bind(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	void bindFirstThread(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	ID3D12Resource& getImage() { return *mImage; };
	const Vector3& position() const { return mLocation.position; }
	const DirectX::XMMATRIX& projectionMatrix() const { return mProjectionMatrix; }
	const Transform& transform() const { return mLocation; }
	Transform& transform() { return mLocation; }
	const Frustum& frustum() const { return mFrustum; }
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView() { return renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }
	unsigned int width() const { return mWidth; }
	unsigned int height() const { return mHeight; }
	unsigned int getShaderResourceViewIndex() const { return shaderResourceViewIndex; }
};