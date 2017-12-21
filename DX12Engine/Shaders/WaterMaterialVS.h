#ifdef __cplusplus
#pragma once
#include <cstdint>
#include <DirectXMath.h>
struct WaterMaterialVS
{
	DirectX::XMMATRIX worldMatrix;
	float waterTranslation;
};
#else
cbuffer Material : register(b1)
{
	matrix worldMatrix;
	float waterTranslation;
};
#endif
