#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "Location.h"
#include "frameBufferCount.h"
#include "Frustum.h"
class BaseExecutor;
class SharedResources;

class Camera
{
protected:
	struct ConstantBuffer
	{
		DirectX::XMMATRIX viewProjectionMatrix;
		DirectX::XMFLOAT3 cameraPosition;
	};
	constexpr static unsigned int constantBufferPerObjectAlignedSize = (sizeof(ConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u)
		& ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);


	ConstantBuffer* constantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpuAddress;
	Location mLocation;
	Frustum mFrustum;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView;
	ID3D12Resource* image;
	unsigned int width, height;
	DirectX::XMMATRIX mProjectionMatrix;

	Camera(const Camera&) = delete;
public:
	constexpr static float screenDepth = 128.0f * 31.5f;
	constexpr static float screenNear = 0.1f;

	Camera() {}
	Camera(SharedResources* sharedResources, ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
		unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
		const Location& target);
	~Camera();

	void update(SharedResources* sharedResources, const DirectX::XMMATRIX& mViewMatrix);
	void makeReflectionLocation(Location& location, float height);
	bool isInView(const Frustum&) { return true; }
	void bind(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	void bindFirstThread(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end);
	ID3D12Resource* getImage();
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView() {return renderTargetView;}
	const ID3D12Resource* getImage() const { return image;}
	DirectX::XMFLOAT3& position() { return mLocation.position; }
	DirectX::XMMATRIX& projectionMatrix() { return mProjectionMatrix; }
	Location& location() { return mLocation; }
	const Frustum& frustum() const { return mFrustum; }
};