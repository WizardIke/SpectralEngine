#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct VtFeedbackCameraMaterial
{
	DirectX::XMMATRIX viewProjectionMatrix;
	float feedbackBias; // log2( feedbackWidth / windowWidth ) + dynamicLODbias
};
#else
cbuffer Camera : register(b0)
{
	matrix viewProjectionMatrix;
	float feedbackBias; // log2( feedbackWidth / windowWidth ) + dynamicLODbias
};
#endif