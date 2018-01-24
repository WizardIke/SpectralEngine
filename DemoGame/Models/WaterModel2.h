#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <frameBufferCount.h>
#include <cstdint>
struct WaterMaterialVS;
class SharedResources;
struct Mesh;

class WaterModel2
{
	uint8_t* vertexConstantBufferCpu;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpu;
	float waterTranslation;
	constexpr static float positionX = 64.0f, positionY = 2.75f, positionZ = 64.0f;
public:
	D3D12_GPU_VIRTUAL_ADDRESS vsConstantBufferGPU(uint32_t frameIndex);
	D3D12_GPU_VIRTUAL_ADDRESS psConstantBufferGPU();
	D3D12_GPU_VIRTUAL_ADDRESS vsAabbGpu();

	Mesh* mesh;

	WaterModel2() {}
	WaterModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int reflectionTextureIndex, unsigned int refractionTextureIndex);
	~WaterModel2() {}

	bool WaterModel2::isInView(const Frustum& Frustum);
	void update(SharedResources& sharedResources);
	float reflectionHeight() { return 2.75f; }
	DirectX::XMFLOAT4 clipPlane() { return DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, positionY + 0.05f); }

	void setNormalTexture(uint32_t diffuseTexture, uint8_t* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress);
};