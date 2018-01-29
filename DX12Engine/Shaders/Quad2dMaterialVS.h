#ifdef __cplusplus
#pragma once
#include <DirectXMath.h>
struct Quad2dMaterialVS
{
	float pos[4u]; //stores position x, position y, width, and height
#ifdef USE_TEXTURE
	float texCoords[4u];
#endif
#ifdef USE_PER_VERTEX_COLOR
	float color[4u];
#endif
};
#else
cbuffer Material : register(b1)
{
	float4 pos : POSITION; //stores position x, position y, width, and height
#ifdef USE_TEXTURE
	float4 texCoords : TEXCOORD;
#endif
#ifdef USE_PER_VERTEX_COLOR
	float4 color : COLOR;
#endif
};
#endif