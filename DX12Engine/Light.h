#pragma once

#include <DirectXMath.h>
#include "PointLight.h"

class Light
{
public:
	Light() {}
	Light(DirectX::XMFLOAT4 ambientLight, DirectX::XMFLOAT4 directionalLight, DirectX::XMFLOAT3 direction) : ambientLight(ambientLight),
		directionalLight(directionalLight), direction(direction) {}

	DirectX::XMFLOAT4 ambientLight;
	DirectX::XMFLOAT4 directionalLight;
	DirectX::XMFLOAT3 direction;
};

class LightConstantBuffer
{
public:
	DirectX::XMFLOAT4 ambientLight;
	DirectX::XMFLOAT4 directionalLight;
	DirectX::XMFLOAT3 lightDirection;
	uint32_t pointLightCount;
	PointLight pointLights[14u];
};