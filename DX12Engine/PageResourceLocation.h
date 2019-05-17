#pragma once
#if defined(__BMI2__) || defined(__AVX2__)
#include <immintrin.h> //_pdep_u64
#endif
#include <cstddef> //std::size_t

class PageResourceLocation
{
#if !(defined(__BMI2__) || defined(__AVX2__))
	static inline unsigned long long splitBy3Bits(unsigned long long a)
	{
		constexpr unsigned long long mask1 = 0x00000000001fffff;
		constexpr unsigned long long mask2 = 0x001f00000000ffff;
		constexpr unsigned long long mask3 = 0x001f0000ff0000ff;
		constexpr unsigned long long mask4 = 0x100f00f00f00f00f;
		constexpr unsigned long long mask5 = 0x10c30c30c30c30c3;
		constexpr unsigned long long mask6 = 0x1249249249249249;

		unsigned long long x = a & mask1;
		x = (x | x << 32) & mask2;
		x = (x | x << 16) & mask3;
		x = (x | x << 8)  & mask4;
		x = (x | x << 4)  & mask5;
		x = (x | x << 2)  & mask6;
		return x;
	}
#endif

	static inline unsigned long long mortonEncode(unsigned long long x, unsigned long long y, unsigned long long z)
	{
#if defined(__BMI2__) || defined(__AVX2__)
		constexpr unsigned long long xMask = 0x9249249249249249;
		constexpr unsigned long long yMask = 0x2492492492492492;
		constexpr unsigned long long zMask = 0x4924924924924924;
		return _pdep_u64(x, xMask) | _pdep_u64(y, yMask) | _pdep_u64(z, zMask);
#else
		return splitBy3Bits(x) | (splitBy3Bits(y) << 1) | (splitBy3Bits(z) << 2);
#endif
	}
public:
	class Hash
	{
	public:
		std::size_t operator()(PageResourceLocation page) const
		{
			return static_cast<std::size_t>(mortonEncode(page.x, page.y, page.mipLevel | (page.textureId << 8u)));
		}
	};

	class HashNoTextureId
	{
	public:
		std::size_t operator()(PageResourceLocation page) const
		{
			return static_cast<std::size_t>(mortonEncode(page.x, page.y, page.mipLevel));
		}
	};

	unsigned short textureId;
	unsigned short mipLevel;
	unsigned short x;
	unsigned short y;

	bool operator==(PageResourceLocation other) const
	{
		return textureId == other.textureId && mipLevel == other.mipLevel && x == other.x && y == other.y;
	}
};