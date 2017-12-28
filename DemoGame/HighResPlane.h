#pragma once
#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include "DirectionalLightMaterialPS.h"

template<unsigned int x, unsigned int z>
class HighResPlane
{
public:
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	D3D12_GPU_VIRTUAL_ADDRESS vsPerObjectCBVGpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;


	constexpr static unsigned int meshIndex = 0u;
	constexpr static float positionX = (float)(x * 128), positionY = 0.0f, positionZ = (float)(z * 128 + 128);

	HighResPlane() {}

	HighResPlane(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int diffuseTextureIndex)
	{
		vsPerObjectCBVGpuAddress = constantBufferGpuAddress;
		constexpr size_t vSPerObjectConstantBufferAlignedSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
		constantBufferGpuAddress += vSPerObjectConstantBufferAlignedSize;

		psPerObjectCBVGpuAddress = constantBufferGpuAddress;
		constexpr size_t pSPerObjectConstantBufferAlignedSize = (sizeof(DirectionalLightMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
		constantBufferGpuAddress += pSPerObjectConstantBufferAlignedSize;

		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		DirectionalLightMaterialPS* psPerObjectCBVCpuAddress = reinterpret_cast<DirectionalLightMaterialPS*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(VSPerObjectConstantBuffer) +
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(psPerObjectCBVCpuAddress) + sizeof(DirectionalLightMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));

		vsPerObjectCBVCpuAddress->worldMatrix = DirectX::XMMatrixTranslation(positionX, positionY, positionZ);

		psPerObjectCBVCpuAddress->baseColorTexture = diffuseTextureIndex;
		psPerObjectCBVCpuAddress->specularColor = { 0.01f, 0.01f, 0.01f };
		psPerObjectCBVCpuAddress->specularPower = 16.0f;
	}
	~HighResPlane() {}

	bool isInView(const Frustum& frustum)
	{
		return frustum.checkCuboid2(positionX + 128.0f, positionY, positionZ + 128.0f, positionX, positionY, positionZ);
	}
};