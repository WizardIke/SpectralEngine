#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
#include <cstdint>
struct FireMaterialPS
{
	DirectX::XMFLOAT2 distortion1;
	DirectX::XMFLOAT2 distortion2;
	DirectX::XMFLOAT2 distortion3;
	float distortionScale;
	float distortionBias;
	uint32_t noiseTexture;
	uint32_t fireTexture;
	uint32_t alphaTexture;
	};
#else
cbuffer Material : register(b1)
{
	float2 distortion1;
	float2 distortion2;
	float2 distortion3;
	float distortionScale;
	float distortionBias;
	uint noiseTexture;
	uint fireTexture;
	uint alphaTexture;
}
#endif
