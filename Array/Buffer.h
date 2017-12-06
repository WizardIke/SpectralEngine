#pragma once
#include <cstddef>

template<class Element, std::size_t capacity>
class alignas(alignof(Element)) Buffer
{
	alignas(alignof(Element)) unsigned char buffer[capacity * sizeof(Element)];
public:
	Element& operator[](std::size_t i)
	{
		return (reinterpret_cast<Element*>(buffer))[i];
	}
	constexpr Element& operator[](std::size_t i) const
	{
		return (reinterpret_cast<const Element*>(buffer))[i];
	}
};