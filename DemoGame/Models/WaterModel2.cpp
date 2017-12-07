#include "WaterModel2.h"
#include "../Assets.h"
#include "../Executor.h"

WaterModel2::WaterModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int reflectionTextureIndex, unsigned int refractionTextureIndex, unsigned int normalTextureIndex) :
	waterTranslation(0.0f)
{
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		vsPerObjectCBVGpuAddresses[i] = constantBufferGpuAddress;
		constantBufferGpuAddress = (constantBufferGpuAddress + sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

		vsPerObjectCBVCpuAddresses[i] = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));
	}

	psPerObjectCBVGpuAddress = constantBufferGpuAddress;
	constantBufferGpuAddress = (constantBufferGpuAddress + sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

	PSPerObjectConstantBuffer* psPerObjectCBVCpuAddress = reinterpret_cast<PSPerObjectConstantBuffer*>(constantBufferCpuAddress);
	constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
		& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));


	psPerObjectCBVCpuAddress->reflectRefractScale = 0.02f;
	psPerObjectCBVCpuAddress->reflectTexture = reflectionTextureIndex;
	psPerObjectCBVCpuAddress->refractTexture = refractionTextureIndex;
	psPerObjectCBVCpuAddress->normalTexture = normalTextureIndex;
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

	assets->mainCamera.makeReflectionMatrix(reflectionMatrix, 2.75f);

	vsPerObjectCBVCpuAddresses[frameIndex]->worldViewProjectionMatrix = DirectX::XMMatrixTranslation(positionX, positionY, positionZ) * assets->mainCamera.viewMatrix * assets->mainCamera.projectionMatrix;
	vsPerObjectCBVCpuAddresses[frameIndex]->worldReflectionProjectionMatrix = DirectX::XMMatrixTranslation(positionX, positionY, positionZ) * reflectionMatrix * assets->mainCamera.projectionMatrix;

	vsPerObjectCBVCpuAddresses[frameIndex]->waterTranslation = waterTranslation;
}