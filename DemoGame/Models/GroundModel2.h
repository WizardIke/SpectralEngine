#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <PointLight.h>

class GroundModel2
{
public:
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	struct PSPerObjectConstantBuffer
	{
		uint32_t baseColorTexture;
	};

	D3D12_GPU_VIRTUAL_ADDRESS vsPerObjectCBVGpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;

	constexpr static unsigned int meshIndex = 1u;
	constexpr static float positionX = 64.0f, positionY = 1.0f, positionZ = 64.0f;

	GroundModel2() {}

	GroundModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int baseColorTextureIndex)
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

		vsPerObjectCBVCpuAddress->worldMatrix = DirectX::XMMATRIX{ 4.f, 0.f, 0.f, 0.f,  0.f, 4.f, 0.f, 0.f,  0.f, 0.f, 4.f, 0.f,  positionX, positionY, positionZ, 1.f };
		psPerObjectCBVCpuAddress->baseColorTexture = baseColorTextureIndex;
	}
	~GroundModel2() {}

	bool GroundModel2::isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 20.0f, positionY, positionZ + 20.0f, positionX - 20.0f, positionY, positionZ - 20.0f);
	}
};