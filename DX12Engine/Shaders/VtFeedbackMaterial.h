#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct VtFeedbackMaterial
{
	float virtualTextureID;
	float textureWidthInPages;
	float textureHeightInPages;
	float usefulTextureWidth; //width of virtual texture not counting padding
	float usefulTextureHeight;
};
#else
cbuffer Material : register(b1)
{
	float virtualTextureID;
	float textureWidthInPages;
	float textureHeightInPages;
	float usefulTextureWidth; //width of virtual texture not counting padding
	float usefulTextureHeight;
};
#endif