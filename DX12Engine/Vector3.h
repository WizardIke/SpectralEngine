#pragma once
#include <cmath>

class Vector3
{
	float mx, my, mz;
public:
	Vector3() {}
	constexpr Vector3(float x, float y, float z) noexcept : mx(x), my(y), mz(z) {}
	constexpr Vector3(const Vector3& other) noexcept = default;

	constexpr float& x() noexcept { return mx; }
	constexpr float& y() noexcept { return my; }
	constexpr float& z() noexcept { return mz; }

	constexpr float x() const noexcept { return mx; }
	constexpr float y() const noexcept { return my; }
	constexpr float z() const noexcept { return mz; }

	constexpr Vector3 operator-(const Vector3& other) const noexcept
	{
		return {mx - other.mx, my - other.my, mz - other.mz};
	}

	constexpr Vector3 operator/(float amount) const noexcept
	{
		return {mx / amount, my / amount, mz / amount};
	}

	constexpr Vector3 operator*(float amount) const noexcept
	{
		return {mx * amount, my * amount, mz * amount};
	}

	constexpr Vector3 operator+(float amount) const noexcept
	{
		return {mx + amount, my + amount, mz + amount};
	}
	
	constexpr Vector3 operator-(float amount) const noexcept
	{
		return {mx - amount, my - amount, mz - amount};
	}

	Vector3 floor() const
	{
		return {std::floor(mx), std::floor(my), std::floor(mz)};
	}

	float length() const
	{
		return std::sqrt(mx * mx + my * my + mz * mz);
	}
};