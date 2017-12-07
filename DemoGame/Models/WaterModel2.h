#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <d3d12.h>
#include <frameBufferCount.h>
class Executor;

class WaterModel2 //uses render to texture with 2 textures
{
public:
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldViewProjectionMatrix;
		DirectX::XMMATRIX worldReflectionProjectionMatrix;
		float waterTranslation;
	};

	struct PSPerObjectConstantBuffer
	{
		float reflectRefractScale;
		uint32_t reflectTexture;
		uint32_t refractTexture;
		uint32_t normalTexture;
	};

	D3D12_GPU_VIRTUAL_ADDRESS vsPerObjectCBVGpuAddresses[frameBufferCount];
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;
	VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddresses[frameBufferCount];

	constexpr static unsigned int meshIndex = 3u;
	constexpr static float positionX = 64.0f, positionY = 2.75f, positionZ = 64.0f;

	float waterTranslation;

	const DirectX::XMFLOAT4 clipPlane = DirectX::XMFLOAT4( 0.0f, -1.0f, 0.0f, positionY + 0.05f);
	DirectX::XMMATRIX reflectionMatrix;

	WaterModel2() {}

	WaterModel2(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int reflectionTextureIndex, unsigned int refractionTextureIndex, unsigned int normalTextureIndex);
	~WaterModel2() {}

	bool WaterModel2::isInView(const Frustum& Frustum);
	void update(Executor* const executor);
};