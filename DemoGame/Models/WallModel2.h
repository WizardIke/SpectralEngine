#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <Light.h>
#include "../Shaders/DirectionalLightMaterialPS.h"

class WallModel2
{
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	constexpr static float positionX = 64.0f, positionY = 6.0f, positionZ = 72.0f;

	constexpr static size_t vsBufferSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t psBufferSize = (sizeof(DirectionalLightMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

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
	
	WallModel2() {}
	WallModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress)
	{
		gpuBuffer = constantBufferGpuAddress;
		constantBufferGpuAddress += vsBufferSize + psBufferSize;

		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress += vsBufferSize;
		auto psPerObjectCBVCpuAddress = reinterpret_cast<DirectionalLightMaterialPS*>(constantBufferCpuAddress);
		constantBufferCpuAddress += psBufferSize;

		vsPerObjectCBVCpuAddress->worldMatrix = {	1.f, 0.f, 0.f, 0.f,
													0.f, 1.f, 0.f, 0.f,
													0.f, 0.f, 1.f, 0.f,
													positionX, positionY, positionZ, 1.f };

		psPerObjectCBVCpuAddress->specularColor = DirectX::XMFLOAT3{ 0.5f, 0.5f, 0.5f};
		psPerObjectCBVCpuAddress->specularPower = 4.0f;
	}
	~WallModel2() {}

	bool WallModel2::isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 5.0f, positionY + 5.0f, positionZ + 0.5f, positionX - 5.0f, positionY - 5.0f, positionZ - 0.5f);
	}

	void setDiffuseTexture(uint32_t diffuseTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<DirectionalLightMaterialPS*>(cpuStartAddress + (gpuBuffer - gpuStartAddress + vsBufferSize));
		buffer->baseColorTexture = diffuseTexture;
	}
};