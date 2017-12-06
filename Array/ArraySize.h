#pragma once
#include <cstddef>

class ArraySize
{
public:
	const std::size_t size;
	ArraySize(std::size_t size) : size(size) {}
};