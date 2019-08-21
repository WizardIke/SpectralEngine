#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <Vector3.h>
#include <cstdint>
class Frustum;
class Mesh;

class CaveModelPart1
{
	constexpr static float pi = 3.141592654f;
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	struct DirectionalLightMaterialPS
	{
		Vector3 specularColor;
		float specularPower;
		uint32_t baseColorTexture;
	};

	constexpr static size_t vSPerObjectConstantBufferAlignedSize = (sizeof(VSPerObjectConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
	constexpr static size_t pSPerObjectConstantBufferAlignedSize = (sizeof(DirectionalLightMaterialPS) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);

	D3D12_GPU_VIRTUAL_ADDRESS gpuBuffer;
public:
	D3D12_GPU_VIRTUAL_ADDRESS vsBufferGpu()
	{
		return gpuBuffer;
	}

	D3D12_GPU_VIRTUAL_ADDRESS psBufferGpu()
	{
		return gpuBuffer + vSPerObjectConstantBufferAlignedSize * numSquares;
	}

	Mesh* mesh;

	constexpr static unsigned int meshIndex = 0u;
	constexpr static unsigned int numSquares = 12u;
	constexpr static float positionX = 64.0f, positionY = -6.0f, positionZ = 91.0f;

	CaveModelPart1() {}

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress);

	~CaveModelPart1() = default;

	bool CaveModelPart1::isInView(const Frustum& Frustum);

	void render(ID3D12GraphicsCommandList* const directCommandList);
	void renderVirtualFeedback(ID3D12GraphicsCommandList* const commandList);

	void setDiffuseTexture(uint32_t diffuseTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<DirectionalLightMaterialPS*>(cpuStartAddress + (gpuBuffer - gpuStartAddress + vSPerObjectConstantBufferAlignedSize * numSquares));
		buffer->baseColorTexture = diffuseTexture;
	}
};