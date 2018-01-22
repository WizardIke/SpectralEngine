#pragma once

#include <DirectXMath.h>

class Transform
{
public:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 rotation;

	DirectX::XMMATRIX toMatrix();
	Transform reflection(float height);
};