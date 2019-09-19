#pragma once

struct ResourceLocation
{
	unsigned long long start;
	unsigned long long end;

	constexpr bool operator==(ResourceLocation other) const noexcept
	{
		return start == other.start && end == other.end;
	}
};