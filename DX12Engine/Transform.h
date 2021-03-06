#pragma once

#include <DirectXMath.h>
#include "Vector3.h"

class Transform
{
public:
	Vector3 position;
	Vector3 rotation;

	DirectX::XMMATRIX toMatrix() const;
	Transform reflection(float height) const;
};