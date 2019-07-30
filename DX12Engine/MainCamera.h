#pragma once
#include "D3D12DescriptorHeap.h"
#include "Shaders/CameraConstantBuffer.h"
#include "Transform.h"
#include "Frustum.h"
#include <DirectXMath.h>
#include "Vector3.h"
#include "frameBufferCount.h"
class Window;
class GraphicsEngine;

class MainCamera
{
	float fieldOfView;
	D3D12DescriptorHeap renderTargetViewDescriptorHeap;
	uint32_t backBufferTextures[frameBufferCount];

	constexpr static unsigned int bufferSizePS = (sizeof(CameraConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u)
		& ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);

	CameraConstantBuffer* constantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpuAddress;
	const Transform& mLocation;
	Frustum mFrustum;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViews[frameBufferCount];
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView;
	ID3D12Resource* mImage;
	unsigned int mWidth, mHeight;
	DirectX::XMMATRIX mProjectionMatrix;
public:
	constexpr static float screenDepth = 128.0f * 31.5f;
	constexpr static float screenNear = 0.1f;

	MainCamera(Window& window, GraphicsEngine& graphicsEngine, float fieldOfView, const Transform& target);
#if defined(_MSC_VER)
	/*
	Initialization of array members doesn't seam to have copy elision in some cases when it should in c++17.
	*/
	MainCamera(MainCamera&&);
#endif

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1);

	void resize(Window& window, GraphicsEngine& graphicsEngine);

	void destruct(GraphicsEngine& graphicsEngine);
	~MainCamera() = default;

	void beforeRender(Window& window, GraphicsEngine& graphicsEngine);
	static constexpr bool isInView() { return true; }
	void bind(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	void bindFirstThread(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	ID3D12Resource& getImage() { return *mImage; };
	const ID3D12Resource& getImage() const { return *mImage; }
	const Vector3& position() const { return mLocation.position; }
	const DirectX::XMMATRIX& projectionMatrix() const { return mProjectionMatrix; }
	const Transform& transform() const { return mLocation; }
	const Frustum& frustum() const { return mFrustum; }
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView(uint32_t frameIndex) const;
	unsigned int width() const { return mWidth; }
	unsigned int height() const { return mHeight; }
};