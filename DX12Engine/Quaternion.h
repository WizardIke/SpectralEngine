#pragma once
class EulerRotation;

class Quaternion
{
public:
	float x, y, z, w;

	Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Quaternion(const EulerRotation& eulerRotation);

	void normalize();
};