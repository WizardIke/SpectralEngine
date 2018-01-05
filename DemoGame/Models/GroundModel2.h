#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <PointLight.h>

class GroundModel2
{
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	struct PSPerObjectConstantBuffer
	{
		uint32_t baseColorTexture;
	};

	D3D12_GPU_VIRTUAL_ADDRESS gpuBuffer;

	constexpr static float positionX = 64.0f, positionY = 1.0f, positionZ = 64.0f;

	constexpr static size_t vsBufferSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t psBufferSize = (sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
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

	GroundModel2() {}
	GroundModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress)
	{
		gpuBuffer = constantBufferGpuAddress;
		constantBufferGpuAddress += vsBufferSize + psBufferSize;

		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress += vsBufferSize + psBufferSize;

		vsPerObjectCBVCpuAddress->worldMatrix = DirectX::XMMATRIX{ 4.f, 0.f, 0.f, 0.f,  0.f, 4.f, 0.f, 0.f,  0.f, 0.f, 4.f, 0.f,  positionX, positionY, positionZ, 1.f };
	}
	~GroundModel2() {}

	bool GroundModel2::isInView(const Frustum& Frustum)
	{
		return Frustum.checkCuboid2(positionX + 20.0f, positionY, positionZ + 20.0f, positionX - 20.0f, positionY, positionZ - 20.0f);
	}

	void setDiffuseTexture(uint32_t diffuseTexture, uint8_t* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<PSPerObjectConstantBuffer*>(cpuStartAddress + (gpuBuffer - gpuStartAddress + vsBufferSize));
		buffer->baseColorTexture = diffuseTexture;
	}
};