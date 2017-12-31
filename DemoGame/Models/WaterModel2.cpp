#include "WaterModel2.h"
#include "../Assets.h"
#include "../Executor.h"
#include <SharedResources.h>
#define USE_REFLECTION_TEXTURE
#include <Shaders/WaterMaterialPS.h>
#undef USE_REFLECTION_TEXTURE
#include <Shaders/WaterMaterialVS.h>

struct AABBMaterial
{
	DirectX::XMMATRIX worldMatrix;
};

constexpr static size_t vertexConstantBufferSize = (sizeof(WaterMaterialVS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
constexpr static size_t pixelConstantBufferSize = (sizeof(WaterMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
constexpr static size_t aabbConstantBufferSize = (sizeof(AABBMaterial) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

WaterModel2::WaterModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int reflectionTextureIndex, unsigned int refractionTextureIndex, unsigned int normalTextureIndex) :
	waterTranslation(0.0f)
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

	psPerObjectCBVCpuAddress->reflectRefractScale = 0.02f;
	psPerObjectCBVCpuAddress->refractTexture = refractionTextureIndex;
	psPerObjectCBVCpuAddress->normalTexture = normalTextureIndex;
	psPerObjectCBVCpuAddress->reflectTexture = reflectionTextureIndex;

	auto aabbBuffer = reinterpret_cast<AABBMaterial*>(constantBufferCpuAddress);
	constantBufferCpuAddress += aabbConstantBufferSize;
	aabbBuffer->worldMatrix = DirectX::XMMatrixScaling(4.1f, 0.05f, 4.1f) * DirectX::XMMatrixTranslation(positionX, positionY, positionZ);
}

bool WaterModel2::isInView(const Frustum& Frustum)
{
	return Frustum.checkCuboid2(positionX + 4.0f, positionY, positionZ + 4.0f, positionX - 4.0f, positionY, positionZ - 4.0f);
}

void WaterModel2::update(Executor* const executor)
{
	auto frameIndex = executor->frameIndex();
	const auto assets = reinterpret_cast<Assets*>(executor->sharedResources);

	waterTranslation += 0.1f * assets->timer.frameTime;
	if (waterTranslation > 1.0f)
	{
		waterTranslation -= 1.0f;
	}

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