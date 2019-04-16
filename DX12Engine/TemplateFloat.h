#pragma once
template<int power, long mantissa>
struct TemplateFloat
{
	constexpr static float value()
	{
		return applyPower((float)mantissa, power);
	}
private:
	constexpr static float applyPower(float value, int power2)
	{
		return power2 == 1.f ? value : power2 > 1.f ? applyPower(value * 2.f, power2 - 1) : applyPower(value / 2.f, power2 + 1);
	}
};

constexpr int getPower(float value, int power = 1)
{
	return (value > 60000000.f || value < -60000000.f) ? getPower(value / 2, power + 1) : (value < 20000000.f && value > -20000000.f) ? getPower(value * 2, power - 1) : power;
}

constexpr float getMantissa(float value)
{
	return (value > 60000000.f || value < -60000000.f) ? getMantissa(value / 2) : (value < 20000000.f && value > -20000000.f) ? getMantissa(value * 2) : value;
}

#define templateFloat(value) TemplateFloat<getPower(value), (long)(getMantissa(value))>