#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct CameraConstantBuffer
{
	DirectX::XMMATRIX viewProjectionMatrix;
	DirectX::XMFLOAT3 cameraPosition;
	};
#else
cbuffer camera : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
}
#endif
