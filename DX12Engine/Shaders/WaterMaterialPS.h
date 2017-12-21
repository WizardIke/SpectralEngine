#ifdef __cplusplus
#pragma once
#include <cstdint>

struct WaterMaterialPS
{
	float reflectRefractScale;
	uint32_t refractTexture;
	uint32_t normalTexture;
#ifdef USE_REFLECTION_TEXTURE
	uint32_t reflectTexture;
#endif
};
#else
cbuffer material : register(b1)
{
	float reflectRefractScale;
	uint refractionTexture;
	uint normalTexture;
#ifdef USE_REFLECTION_TEXTURE
	uint reflectionTexture;
#endif
};
#endif