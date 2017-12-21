#ifdef __cplusplus
#pragma once
#include <cstdint>
struct GlassMaterialPS
{
	uint32_t diffuseTexture;
	uint32_t normalTexture;
	uint32_t refractionTexture;
	float refractionScale;
	float opacity;
};
#else
cbuffer Material : register(b1)
{
	uint diffuseTexture;
	uint normalTexture;
	uint refractionTexture;
	float refractionScale;
	float opacity;
};
#endif
