#pragma once
#include <DirectXMath.h>
#include <cstdint>

#define USE_BASE_COLOR_TEXTURE

struct BasicMaterialPS
{
#ifdef USE_PER_OBJECT_BASE_COLOR
	DirectX::XMFLOAT4 baseColor;
#endif
#ifdef USE_PER_OBJECT_SPECULAR
	DirectX::XMFLOAT3 specularColor;
	float specularPower;
#endif
#ifdef USE_PER_OBJECT_EMMISIVE_LIGHT
	DirectX::XMFLOAT4 emmisiveColor;
#endif

#if defined(USE_PER_OBJECT_MINREFLECTANCE)
	float minReflectance;
#endif

#ifdef USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
	DirectX::XMFLOAT3 minReflectance;
#endif

#ifdef USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS
	float roughnessAsMeanQuareSlopeOfMicrofacets;
#endif

#ifdef USE_BASE_COLOR_TEXTURE
	uint32_t baseColorTexture;
#endif

#ifdef USE_SPECULAR_TEXTURE
	uint32_t specularTexture;
#endif

#ifdef USE_AMBIENT_TEXTURE
	uint32_t ambientTexture;
#endif

#ifdef USE_NORAML_TEXTURE
	uint32_t normalTexture;
#endif

#ifdef USE_EMMISIVE_TEXTURE
	uint32_t emmisiveTexture;
#endif
};
#undef USE_BASE_COLOR_TEXTURE