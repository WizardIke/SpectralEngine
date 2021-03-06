#include "WaterModel2.h"
#define USE_REFLECTION_TEXTURE
#include <Shaders/WaterMaterialPS.h>
#undef USE_REFLECTION_TEXTURE
#include <Shaders/WaterMaterialVS.h>
#include <GraphicsEngine.h>

namespace
{
	struct AABBMaterial
	{
		DirectX::XMMATRIX worldMatrix;
	};

	constexpr static size_t vertexConstantBufferSize = (sizeof(WaterMaterialVS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t pixelConstantBufferSize = (sizeof(WaterMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t aabbConstantBufferSize = (sizeof(AABBMaterial) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
}

void WaterModel2::setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress, unsigned int reflectionTextureIndex, unsigned int refractionTextureIndex)
{
	vertexConstantBufferCpu = constantBufferCpuAddress;
	constantBufferGpu = constantBufferGpuAddress;
	constantBufferGpuAddress += frameBufferCount * vertexConstantBufferSize + pixelConstantBufferSize + aabbConstantBufferSize;
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		auto buffer = reinterpret_cast<WaterMaterialVS*>(constantBufferCpuAddress);
		buffer->worldMatrix = DirectX::XMMatrixTranslation(positionX, positionY, positionZ);
		constantBufferCpuAddress += vertexConstantBufferSize;
	}
	WaterMaterialPS* psPerObjectCBVCpuAddress = reinterpret_cast<WaterMaterialPS*>(constantBufferCpuAddress);
	constantBufferCpuAddress += pixelConstantBufferSize;

	psPerObjectCBVCpuAddress->reflectRefractScale = 0.2f;
	psPerObjectCBVCpuAddress->refractTexture = refractionTextureIndex;
	psPerObjectCBVCpuAddress->reflectTexture = reflectionTextureIndex;

	auto aabbBuffer = reinterpret_cast<AABBMaterial*>(constantBufferCpuAddress);
	constantBufferCpuAddress += aabbConstantBufferSize;
	aabbBuffer->worldMatrix = DirectX::XMMatrixScaling(4.1f, 0.05f, 4.1f) * DirectX::XMMatrixTranslation(positionX, positionY, positionZ);
}

bool WaterModel2::isInView(const Frustum& Frustum)
{
	return Frustum.checkCuboid2(positionX + 4.0f, positionY, positionZ + 4.0f, positionX - 4.0f, positionY, positionZ - 4.0f);
}

void WaterModel2::update(float frameTime)
{
	waterTranslation += 0.1f * frameTime;
	if (waterTranslation > 1.0f)
	{
		waterTranslation -= 1.0f;
	}
}

void WaterModel2::beforeRender(GraphicsEngine& graphicsEngine)
{
	auto frameIndex = graphicsEngine.frameIndex;
	auto buffer = reinterpret_cast<WaterMaterialVS*>(vertexConstantBufferCpu + frameIndex * vertexConstantBufferSize);
	buffer->waterTranslation = waterTranslation;
}

D3D12_GPU_VIRTUAL_ADDRESS WaterModel2::vsConstantBufferGPU(uint32_t frameIndex)
{
	return constantBufferGpu + frameIndex * vertexConstantBufferSize;
}

D3D12_GPU_VIRTUAL_ADDRESS WaterModel2::psConstantBufferGPU() 
{
	return constantBufferGpu + frameBufferCount * vertexConstantBufferSize; 
};

D3D12_GPU_VIRTUAL_ADDRESS WaterModel2::vsAabbGpu()
{
	return constantBufferGpu + frameBufferCount * vertexConstantBufferSize + pixelConstantBufferSize;
}

void WaterModel2::setNormalTexture(uint32_t normalTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
{
	auto buffer = reinterpret_cast<WaterMaterialPS*>(cpuStartAddress + (constantBufferGpu - gpuStartAddress + vertexConstantBufferSize * frameBufferCount));
	buffer->normalTexture = normalTexture;
}