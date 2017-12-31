#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <Shaders/GlassMaterialPS.h>

class IceModel2
{
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpu;
	constexpr static float positionX = 69.0f, positionY = 5.0f, positionZ = 54.0f;

	constexpr static size_t vsConstantBufferSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t psConstantBufferSize = (sizeof(GlassMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
public:
	D3D12_GPU_VIRTUAL_ADDRESS vsConstantBufferGPU(uint32_t frameIndex)
	{
		return constantBufferGpu;
	}

	D3D12_GPU_VIRTUAL_ADDRESS psConstantBufferGPU() { return constantBufferGpu + vsConstantBufferSize; };

	D3D12_GPU_VIRTUAL_ADDRESS vsAabbGpu() { return constantBufferGpu + vsConstantBufferSize + psConstantBufferSize; }
	D3D12_GPU_VIRTUAL_ADDRESS psAabbGpu() { return constantBufferGpu + vsConstantBufferSize + psConstantBufferSize + vsConstantBufferSize; }

	constexpr static unsigned int meshIndex = 4u;

	IceModel2() {}

	IceModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int refractionTextureIndex, unsigned int diffuseTextureIndex, unsigned int normalTextureIndex)
	{
		constantBufferGpu = constantBufferGpuAddress;
		constantBufferGpuAddress += vsConstantBufferSize + vsConstantBufferSize + psConstantBufferSize;
		auto vsBuffer = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		vsBuffer->worldMatrix = DirectX::XMMATRIX(5.f, 0.f, 0.f, 0.f, 0.f, 5.f, 0.f, 0.f, 0.f, 0.f, 5.f, 0.f, positionX, positionY, positionZ, 1.f);
		constantBufferCpuAddress += vsConstantBufferSize;
		auto psBuffer = reinterpret_cast<GlassMaterialPS*>(constantBufferCpuAddress);
		psBuffer->refractionScale = 0.1f;
		psBuffer->opacity = 0.2f;
		psBuffer->refractionTexture = refractionTextureIndex;
		psBuffer->diffuseTexture = diffuseTextureIndex;
		psBuffer->normalTexture = normalTextureIndex;
		constantBufferCpuAddress += psConstantBufferSize;
		auto aabbBuffer = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		aabbBuffer->worldMatrix = DirectX::XMMatrixScaling(5.1f, 5.1f, 5.1f) * DirectX::XMMatrixTranslation(positionX, positionY, positionZ);
		constantBufferCpuAddress += vsConstantBufferSize;
	}
	~IceModel2() {}

	bool IceModel2::isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 5.0f, positionY + 5.0f, positionZ + 5.0f, positionX - 5.0f, positionY - 5.0f, positionZ - 5.0f);
	}
};