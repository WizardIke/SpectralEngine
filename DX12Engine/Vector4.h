#pragma once

class Vector4
{
	float mx, my, mz, mw;
public:
	Vector4() {}
	Vector4(float x, float y, float z, float w) : mx(x), my(y), mz(z), mw(w) {}
	Vector4(const Vector4& other) = default;

	float& x() { return mx; }
	float& y() { return my; }
	float& z() { return mz; }
	float& w() { return mw; }
};