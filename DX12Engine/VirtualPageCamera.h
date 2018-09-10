#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "Transform.h"
#include "frameBufferCount.h"
#include "Frustum.h"
#include "Shaders/VtFeedbackCameraMaterial.h"
#include <new>
#include "CameraUtil.h"
class D3D12GraphicsEngine;

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
	VirtualPageCamera(ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
		unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1, float fieldOfView,
		Transform& target);
	void init(ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
		unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1, float fieldOfView,
		Transform& target)
	{
		this->~VirtualPageCamera();
		new(this) VirtualPageCamera(image, renderTargetView, depthSencilView, width, height, constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView, target);
	}
	~VirtualPageCamera();

	void update(const D3D12GraphicsEngine& graphicsEngine, float mipBias);
	bool isInView() const { return true; }
	void bind(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
	{
		auto constantBufferGPU = constantBufferGpuAddress + bufferSizePS * frameIndex;
		CameraUtil::bind(first, end, CameraUtil::getViewPort(mWidth, mHeight), CameraUtil::getScissorRect(mWidth, mHeight), constantBufferGPU, &renderTargetView, &depthSencilView);
	}
	
	void bindFirstThread(uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
	{
		bind(frameIndex, first, end);
		auto commandList = *first;
		constexpr float clearColor[] = { 0.0f, 0.0f, 65280.0f, 65535.0f };
		commandList->ClearRenderTargetView(renderTargetView, clearColor, 0u, nullptr);
		commandList->ClearDepthStencilView(depthSencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, (unsigned char)0u, 0u, nullptr);
	}
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView() { return renderTargetView; }
	ID3D12Resource* getImage() { return mImage; }
	const ID3D12Resource* getImage() const { return mImage; }
	DirectX::XMMATRIX& projectionMatrix() { return mProjectionMatrix; }
	template<class Void>
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView(Void& notUsed) { return renderTargetView; }
	unsigned int width() { return mWidth; }
	unsigned int height() { return mHeight; }
};