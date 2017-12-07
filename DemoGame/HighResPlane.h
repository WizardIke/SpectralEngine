#pragma once
#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>

template<unsigned int x, unsigned int z>
class HighResPlane
{
public:
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	struct PSPerObjectConstantBuffer
	{
		DirectX::XMFLOAT4 specularColor;
		float specularPower;
		UINT32 diffuseTexture;
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
		constexpr size_t pSPerObjectConstantBufferAlignedSize = (sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
		constantBufferGpuAddress += pSPerObjectConstantBufferAlignedSize;

		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		PSPerObjectConstantBuffer* psPerObjectCBVCpuAddress = reinterpret_cast<PSPerObjectConstantBuffer*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(VSPerObjectConstantBuffer) +
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(psPerObjectCBVCpuAddress) + sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));

		vsPerObjectCBVCpuAddress->worldMatrix = DirectX::XMMatrixTranslation(positionX, positionY, positionZ);

		psPerObjectCBVCpuAddress->specularColor = { 0.5f, 0.5f, 0.5f, 1.0f };
		psPerObjectCBVCpuAddress->specularPower = 4.0f;
		psPerObjectCBVCpuAddress->diffuseTexture = diffuseTextureIndex;
	}
	~HighResPlane() {}

	bool isInView(const Frustum& frustum)
	{
		return frustum.checkCuboid2(positionX + 128.0f, positionY, positionZ + 128.0f, positionX, positionY, positionZ);
	}
};