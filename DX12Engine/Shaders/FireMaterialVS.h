#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct FireMaterialVS
{
	DirectX::XMMATRIX worldMatrix;
	float flameTime;
	DirectX::XMFLOAT3 scrollSpeeds;
	DirectX::XMFLOAT3 scales;
};
#else
cbuffer Material : register(b1)
{
	matrix worldMatrix;
	float flameTime;
	float3 scrollSpeeds;
	float3 scales;
}
#endif
