#pragma once

#include <DirectXMath.h>
#include "D3D12Resource.h"
class BaseExecutor;
struct ID3D12GraphicsCommandList;

class PerFramConstantBuffers
{
	struct CameraConstantBuffer
	{
		DirectX::XMMATRIX viewProjectionMatrix;
		DirectX::XMFLOAT3 cameraPosition;
	};

	constexpr static unsigned int constantBufferPerObjectAlignedSize = (sizeof(CameraConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
	CameraConstantBuffer* cameraConstantBufferCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS cameraConstantBufferGpuAddress;
public:
	PerFramConstantBuffers() {}
	PerFramConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress);
	~PerFramConstantBuffers();

	void update(BaseExecutor* const executor);
	void bind(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);
};