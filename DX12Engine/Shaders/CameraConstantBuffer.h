#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct CameraConstantBuffer
{
	DirectX::XMMATRIX viewProjectionMatrix;
	DirectX::XMFLOAT3 cameraPosition;
	float screenWidth;
	float screenHeight;
	};
#else
cbuffer camera : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
	float screenWidth;
	float screenHeight;
}
#endif
