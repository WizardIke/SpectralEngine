#pragma once
#include "ArrayBase.h"

template<class Element>
class ArrayPointer : public ArrayBase<Element>
{
public:
	ArrayPointer(Element* const buffer, const std::size_t capacity) : buffer(buffer), size(capacity) {}
	ArrayPointer(const ArrayPointer&) = default;
	ArrayPointer(ArrayPointer&&) = default;

	constexpr bool empty() const noexcept
	{
		return capacity == 0;
	}

	void swap(ArrayPointer& other) // noexcept(noexcept(std::swap(declval<ElementType&>(), declval<ElementType&>())))
	{
		union
		{
			std::size_t size;
			Element* buffer;
		};
		size = this->size;
		this->size = other.size;
		other.size = size;

		buffer = this->buffer;
		this->buffer = other.buffer;
		other.buffer = buffer;
	}
	const ArrayPointer& operator=(const ArrayPointer other)
	{
		this->size = other.size;
		this->buffer = other.buffer;
		return *this;
	}
};

template<class Element>
void swap(ArrayPointer<Element>& lhs, ArrayPointer<Element>& rhs)
{
	lhs.swap(rhs);
}