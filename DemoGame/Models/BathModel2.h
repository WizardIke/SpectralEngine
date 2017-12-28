#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <Light.h>
#include "../DirectionalLightMaterialPS.h"

class BathModel2
{
public:
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	D3D12_GPU_VIRTUAL_ADDRESS vsPerObjectCBVGpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;
	

	constexpr static unsigned int meshIndex = 0u;
	constexpr static float positionX = 64.0f, positionY = 2.0f, positionZ = 64.0f;

	BathModel2() {}

	BathModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int baseColorTextureIndex)
	{
		vsPerObjectCBVGpuAddress = constantBufferGpuAddress;
		psPerObjectCBVGpuAddress = (constantBufferGpuAddress + sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
		constantBufferGpuAddress = (psPerObjectCBVGpuAddress + sizeof(DirectionalLightMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	
		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));
		DirectionalLightMaterialPS* psPerObjectCBVCpuAddress = reinterpret_cast<DirectionalLightMaterialPS*>(constantBufferCpuAddress);
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(DirectionalLightMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));

		vsPerObjectCBVCpuAddress->worldMatrix = {	1.f, 0.f, 0.f, 0.f,
													0.f, 1.f, 0.f, 0.f,
													0.f, 0.f, 1.f, 0.f,
													positionX, positionY, positionZ, 1.f };

		psPerObjectCBVCpuAddress->specularColor = DirectX::XMFLOAT3{ 0.1f, 0.1f, 0.1f};
		psPerObjectCBVCpuAddress->specularPower = 4.0f;
		psPerObjectCBVCpuAddress->baseColorTexture = baseColorTextureIndex;
	}
	~BathModel2() {}

	bool isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 5.f, positionY + 1.f, positionZ + 5.f, positionX - 5.f, positionY - 1.f, positionZ - 5.f);
	}
};