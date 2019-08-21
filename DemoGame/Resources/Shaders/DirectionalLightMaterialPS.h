#pragma once
// USE_DIRECTIONAL_LIGHT
// USE_PER_OBJECT_SPECULAR
// USE_PER_OBJECT_AMBIENT
// USE_BASE_COLOR_TEXTURE
#include <DirectXMath.h>
#include <cstdint>

struct DirectionalLightMaterialPS
{
	DirectX::XMFLOAT3 specularColor;
	float specularPower;
	uint32_t baseColorTexture;
};