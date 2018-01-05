#pragma once
#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include "DirectionalLightMaterialPS.h"

template<unsigned int x, unsigned int z>
class HighResPlane
{
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	constexpr static size_t vsBufferSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t psBufferSize = (sizeof(DirectionalLightMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

	D3D12_GPU_VIRTUAL_ADDRESS gpuBuffer;

	constexpr static float positionX = (float)(x * 128), positionY = 0.0f, positionZ = (float)(z * 128 + 128);
public:
	D3D12_GPU_VIRTUAL_ADDRESS vsBufferGpu()
	{
		return gpuBuffer;
	}

	D3D12_GPU_VIRTUAL_ADDRESS psBufferGpu()
	{
		return gpuBuffer + vsBufferSize;
	}

	Mesh* mesh;

	HighResPlane() {}
	HighResPlane(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress)
	{
		gpuBuffer = constantBufferGpuAddress;
		constantBufferGpuAddress += vsBufferSize + psBufferSize;

		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress += vsBufferSize;
		DirectionalLightMaterialPS* psPerObjectCBVCpuAddress = reinterpret_cast<DirectionalLightMaterialPS*>(constantBufferCpuAddress);
		constantBufferCpuAddress += psBufferSize;

		vsPerObjectCBVCpuAddress->worldMatrix = DirectX::XMMatrixTranslation(positionX, positionY, positionZ);

		psPerObjectCBVCpuAddress->specularColor = { 0.01f, 0.01f, 0.01f };
		psPerObjectCBVCpuAddress->specularPower = 16.0f;
	}
	~HighResPlane() {}

	bool isInView(const Frustum& frustum)
	{
		return frustum.checkCuboid2(positionX + 128.0f, positionY, positionZ + 128.0f, positionX, positionY, positionZ);
	}

	void setDiffuseTexture(uint32_t diffuseTexture, uint8_t* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<DirectionalLightMaterialPS*>(cpuStartAddress + (gpuBuffer - gpuStartAddress + vsBufferSize));
		buffer->baseColorTexture = diffuseTexture;
	}
};