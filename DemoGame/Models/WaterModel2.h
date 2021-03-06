#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <frameBufferCount.h>
#include <cstdint>
struct WaterMaterialVS;
class Mesh;
class GraphicsEngine;

class WaterModel2
{
	unsigned char* vertexConstantBufferCpu;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpu;
	float waterTranslation = 0.0f;
	constexpr static float positionX = 64.0f, positionY = 2.75f, positionZ = 64.0f;
public:
	D3D12_GPU_VIRTUAL_ADDRESS vsConstantBufferGPU(uint32_t frameIndex);
	D3D12_GPU_VIRTUAL_ADDRESS psConstantBufferGPU();
	D3D12_GPU_VIRTUAL_ADDRESS vsAabbGpu();

	Mesh* mesh;

	WaterModel2() {}

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress, unsigned int reflectionTextureIndex, unsigned int refractionTextureIndex);

	~WaterModel2() = default;

	bool WaterModel2::isInView(const Frustum& Frustum);
	void update(float frameTime);
	void beforeRender(GraphicsEngine& graphicsEngine);
	float reflectionHeight() { return 2.75f; }
	DirectX::XMFLOAT4 clipPlane() { return DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, positionY + 0.05f); }

	void setNormalTexture(uint32_t diffuseTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress);
};