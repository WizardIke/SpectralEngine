#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct VtFeedbackMaterialPS
{
	float virtualTextureID1;
	float virtualTextureID2And3;
	float textureWidthInPages;
	float textureHeightInPages;
	float usefulTextureWidth; //width of virtual texture not counting padding
	float usefulTextureHeight;
};
#else
cbuffer Material : register(b1)
{
	float virtualTextureID1;
	float virtualTextureID2And3;
	float textureWidthInPages;
	float textureHeightInPages;
	float usefulTextureWidth; //width of virtual texture not counting padding
	float usefulTextureHeight;
};
#endif