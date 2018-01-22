#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct VtFeedbackMaterial
{
	float virtualTextureID;
};
#else
cbuffer Material : register(b1)
{
	float virtualTextureID;
};
#endif