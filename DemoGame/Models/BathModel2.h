#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <Light.h>
#include "../Shaders/DirectionalLightMaterialPS.h"

class BathModel2
{
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	constexpr static float positionX = 64.0f, positionY = 2.0f, positionZ = 64.0f;
	constexpr static size_t psBufferSize = (sizeof(DirectionalLightMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t vsBufferSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

	D3D12_GPU_VIRTUAL_ADDRESS gpuBuffer;
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

	BathModel2() {}

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress)
	{
		gpuBuffer = constantBufferGpuAddress;
		constantBufferGpuAddress += psBufferSize + vsBufferSize;

		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress += vsBufferSize;
		DirectionalLightMaterialPS* psPerObjectCBVCpuAddress = reinterpret_cast<DirectionalLightMaterialPS*>(constantBufferCpuAddress);
		constantBufferCpuAddress += psBufferSize;

		vsPerObjectCBVCpuAddress->worldMatrix = { 1.f, 0.f, 0.f, 0.f,
													0.f, 1.f, 0.f, 0.f,
													0.f, 0.f, 1.f, 0.f,
													positionX, positionY, positionZ, 1.f };

		psPerObjectCBVCpuAddress->specularColor = DirectX::XMFLOAT3{ 0.1f, 0.1f, 0.1f };
		psPerObjectCBVCpuAddress->specularPower = 4.0f;
	}

	~BathModel2() = default;

	bool isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 5.f, positionY + 1.f, positionZ + 5.f, positionX - 5.f, positionY - 1.f, positionZ - 5.f);
	}

	void setDiffuseTexture(uint32_t diffuseTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<DirectionalLightMaterialPS*>(cpuStartAddress + (gpuBuffer - gpuStartAddress + vsBufferSize));
		buffer->baseColorTexture = diffuseTexture;
	}
};