#include "Quaternion.h"
#include "EulerRotation.h"
#include <cmath>

Quaternion::Quaternion(const EulerRotation& eulerRotation)
{
	float halfYaw = eulerRotation.yaw * 0.5f;
	float halfPitch = eulerRotation.pitch * 0.5f;
	float halfRoll = eulerRotation.roll * 0.5f;

	float cosYaw = std::cosf(halfYaw);
	float sinYaw = std::sinf(halfYaw);
	float cosPitch = std::cosf(halfPitch);
	float sinPitch = std::sinf(halfPitch);
	float cosRoll = std::cosf(halfRoll);
	float sinRoll = std::sinf(halfRoll);

	float cycp = cosYaw * cosPitch;
	float sysp = sinYaw * sinPitch;
	float sycp = sinYaw * cosPitch;
	float cysp = cosYaw * sinPitch;

	x = cosRoll * cysp + sinRoll * sycp;
	y = cosRoll * sycp - sinRoll * cysp;
	z = sinRoll * cycp - cosRoll * sysp;
	w = cosRoll * cycp + sinRoll * sysp;
}

void Quaternion::normalize()
{
	float oneOverLength = std::sqrt(x * x + y * y + z * z + w * w);
	if (oneOverLength == 0.0f)
	{
		w = 1.0f;
		return;
	}
	oneOverLength = 1.0f / oneOverLength;
	x *= oneOverLength;
	y *= oneOverLength;
	z *= oneOverLength;
	w *= oneOverLength;
}