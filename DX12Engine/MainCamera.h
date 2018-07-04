#pragma once
#include "D3D12DescriptorHeap.h"
#include "Shaders/CameraConstantBuffer.h"
#include "Transform.h"
#include "Frustum.h"
#include <DirectXMath.h>
#include "frameBufferCount.h"
#include <new>
class Window;
class D3D12GraphicsEngine;

class MainCamera
{
	D3D12DescriptorHeap renderTargetViewDescriptorHeap;
	uint32_t backBufferTextures[frameBufferCount];

	constexpr static unsigned int bufferSizePS = (sizeof(CameraConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u)
		& ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);


	CameraConstantBuffer* constantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpuAddress;
	Transform mLocation;
	Frustum mFrustum;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViews[frameBufferCount];
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView;
	ID3D12Resource* mImage;
	unsigned int mWidth, mHeight;
	DirectX::XMMATRIX mProjectionMatrix;
public:
	constexpr static float screenDepth = 128.0f * 31.5f;
	constexpr static float screenNear = 0.1f;

	MainCamera() {}
	MainCamera(Window& window, D3D12GraphicsEngine& graphicsEngine, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
		uint8_t*& constantBufferCpuAddress1, float fieldOfView, const Transform& target);
	void init(Window& window, D3D12GraphicsEngine& graphicsEngine, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
		uint8_t*& constantBufferCpuAddress1, float fieldOfView, const Transform& target)
	{
		this->~MainCamera();
		new(this) MainCamera(window, graphicsEngine, width, height, constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView, target);
	}

	void destruct(D3D12GraphicsEngine& graphicsEngine);
	~MainCamera();

	void update(Window& window, D3D12GraphicsEngine& graphicsEngine, const Transform& target);
	bool isInView() const { return true; }
	void bind(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	void bindFirstThread(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	ID3D12Resource* getImage() { return mImage; };
	const ID3D12Resource* getImage() const { return mImage; }
	DirectX::XMFLOAT3& position() { return mLocation.position; }
	DirectX::XMMATRIX& projectionMatrix() { return mProjectionMatrix; }
	Transform& transform() { return mLocation; }
	const Frustum& frustum() const { return mFrustum; }
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView(uint32_t frameIndex);
	unsigned int width() { return mWidth; }
	unsigned int height() { return mHeight; }
};