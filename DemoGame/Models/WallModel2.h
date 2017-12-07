#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <Light.h>

class WallModel2
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
		uint32_t baseColorTexture;
	};

	D3D12_GPU_VIRTUAL_ADDRESS vsPerObjectCBVGpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;

	constexpr static unsigned int meshIndex = 2u;
	constexpr static float positionX = 64.0f, positionY = 6.0f, positionZ = 72.0f;

	WallModel2() {}

	WallModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int baseColorTextureIndex)
	{
		vsPerObjectCBVGpuAddress = constantBufferGpuAddress;
		constantBufferGpuAddress = (constantBufferGpuAddress + sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
		psPerObjectCBVGpuAddress = constantBufferGpuAddress;
		constantBufferGpuAddress = (constantBufferGpuAddress + sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));
		PSPerObjectConstantBuffer* psPerObjectCBVCpuAddress = reinterpret_cast<PSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));

		vsPerObjectCBVCpuAddress->worldMatrix = {	1.f, 0.f, 0.f, 0.f,
													0.f, 1.f, 0.f, 0.f,
													0.f, 0.f, 1.f, 0.f,
													positionX, positionY, positionZ, 1.f };

		psPerObjectCBVCpuAddress->specularColor = DirectX::XMFLOAT4{ 0.5f, 0.5f, 0.5f, 1.0f };
		psPerObjectCBVCpuAddress->specularPower = 4.0f;
		psPerObjectCBVCpuAddress->baseColorTexture = baseColorTextureIndex;
	}
	~WallModel2() {}

	bool WallModel2::isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 5.0f, positionY + 5.0f, positionZ + 0.5f, positionX - 5.0f, positionY - 5.0f, positionZ - 0.5f);
	}
};