#pragma once
#include <stdint.h>

class Area
{
public:
	class VisitedNode
	{
	public:
		void* thisArea;
		VisitedNode* next;
	};

	static constexpr int32_t constexprCeil(float num)
	{
		return (static_cast<float>(static_cast<int32_t>(num)) == num)
			? static_cast<int32_t>(num)
			: static_cast<int32_t>(num) + ((num > 0) ? 1 : 0);
	}

	constexpr static float zoneDiameter = 128.0f;
};