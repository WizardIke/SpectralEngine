#pragma once
class Quaternion;

class EulerRotation
{
public:
	float roll, pitch, yaw;

	EulerRotation(float roll, float pitch, float yaw) : roll(roll), pitch(pitch), yaw(yaw) {}
	EulerRotation(Quaternion& quaternion);
};