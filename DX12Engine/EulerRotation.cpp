#include "EulerRotation.h"
#include "Quaternion.h"
#include <cmath>

EulerRotation::EulerRotation(Quaternion& quaternion)
{
	float ysqr = quaternion.y * quaternion.y;

	// roll (x-axis rotation)
	float t0 = +2.0f * (quaternion.w * quaternion.x + quaternion.y * quaternion.z);
	float t1 = +1.0f - 2.0f * (quaternion.x * quaternion.x + ysqr);
	roll = std::atan2(t0, t1);

	// pitch (y-axis rotation)
	float t2 = +2.0f * (quaternion.w * quaternion.y - quaternion.z * quaternion.x);
	t2 = ((t2 > 1.0f) ? 1.0f : t2);
	t2 = ((t2 < -1.0f) ? -1.0f : t2);
	pitch = std::asin(t2);

	// yaw (z-axis rotation)
	float t3 = +2.0f * (quaternion.w * quaternion.z + quaternion.x * quaternion.y);
	float t4 = +1.0f - 2.0f * (ysqr + quaternion.z * quaternion.z);
	yaw = std::atan2(t3, t4);
}