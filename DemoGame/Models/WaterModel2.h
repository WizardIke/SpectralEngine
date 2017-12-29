#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <frameBufferCount.h>
#include <cstdint>
class Executor;
struct WaterMaterialVS;

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

	constexpr static unsigned int meshIndex = 3u;

	WaterModel2() {}
	WaterModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int reflectionTextureIndex, unsigned int refractionTextureIndex, unsigned int normalTextureIndex);
	~WaterModel2() {}

	bool WaterModel2::isInView(const Frustum& Frustum);
	void update(Executor* const executor);
	float reflectionHeight() { return 2.75f; }
	DirectX::XMFLOAT4 clipPlane() { return DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, positionY + 0.05f); }
};