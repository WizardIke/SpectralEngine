#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>

class IceModel2
{
public:
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	struct PSPerObjectConstantBuffer
	{
		float refractionScale;
		uint32_t refractionTexture;
		uint32_t diffuseTexture;
		uint32_t normalTexture;
	};

	D3D12_GPU_VIRTUAL_ADDRESS vsPerObjectCBVGpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;

	constexpr static unsigned int meshIndex = 4u;
	constexpr static float positionX = 69.0f, positionY = 5.0f, positionZ = 54.0f;

	IceModel2() {}

	IceModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int refractionTextureIndex, unsigned int diffuseTextureIndex, unsigned int normalTextureIndex)
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


		vsPerObjectCBVCpuAddress->worldMatrix = DirectX::XMMATRIX(5.f, 0.f, 0.f, 0.f,  0.f, 5.f, 0.f, 0.f,  0.f, 0.f, 5.f, 0.f, positionX, positionY, positionZ, 1.f);
		psPerObjectCBVCpuAddress->refractionScale = 0.1f;
		psPerObjectCBVCpuAddress->refractionTexture = refractionTextureIndex;
		psPerObjectCBVCpuAddress->diffuseTexture = diffuseTextureIndex;
		psPerObjectCBVCpuAddress->normalTexture = normalTextureIndex;
	}
	~IceModel2() {}

	bool IceModel2::isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 5.0f, positionY + 5.0f, positionZ + 5.0f, positionX - 5.0f, positionY - 5.0f, positionZ - 5.0f);
	}
};