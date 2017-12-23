#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <algorithm>

template<class Element>
class ArrayBase
{
protected:
	Element* buffer;
	std::size_t mCapacity;

	template<std::size_t size, std::size_t alignment>
	struct alignas(alignment)Memory
	{
		unsigned char padding[size];
	};
public:
	using value_type = Element;
	using iterator = Element*;
	using const_iterator = const Element*;
	using reverse_iterator = ::std::reverse_iterator<iterator>;
	using const_reverse_iterator = ::std::reverse_iterator<const_iterator>;
	using pointer = value_type*;
	using reference = value_type&;
	using const_pointer = const pointer;
	using const_reference = const reference;
	using size_type = std::size_t;

	ArrayBase(Element* const buffer, const std::size_t capacity) : buffer(buffer), mCapacity(capacity) {}
	ArrayBase(const ArrayBase&) = default;

	Element& at(const std::size_t pos)
	{
		using namespace std::string_literals;

		if (pos >= size)
		{
			std::string message = "Index "s;
			message += pos;
			message += "is out of range, the Array's size is "s;
			message += size;
			throw std::out_of_range(message);
		}
		return buffer[pos];
	}
	constexpr Element& at(const std::size_t pos) const
	{
		using namespace std::string_literals;

		if (pos >= size)
		{
			std::string message = "Index "s;
			message += pos;
			message += "is out of range, the Array's size is "s;
			message += size;
			throw std::out_of_range(message);
		}
		return buffer[pos];
	}
	Element& operator[](const std::size_t pos)
	{
		return buffer[pos];
	}
	const Element& operator[](const std::size_t pos) const
	{
		return buffer[pos];
	}
	constexpr Element& get(const std::size_t pos) const
	{
		return *(buffer + pos);
	}
	Element& front()
	{
		return buffer[0];
	}
	Element& back()
	{
		return buffer[size - 1u];
	}
	Element* data()
	{
		return buffer;
	}

	iterator begin() noexcept
	{
		return buffer;
	}

	const_iterator begin() const noexcept
	{
		return buffer;
	}
	constexpr iterator cbegin() const noexcept
	{
		return buffer;
	}
	reverse_iterator rbegin() noexcept
	{
		return reverse_iterator(buffer);
	}
	const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator(buffer);
	}
	iterator end() noexcept
	{
		return &buffer[mCapacity];
	}
	const_iterator end() const noexcept
	{
		return &buffer[mCapacity];
	}
	constexpr iterator cend() const noexcept
	{
		return &buffer[mCapacity];
	}
	reverse_iterator  rend() noexcept
	{
		return reverse_iterator(&buffer[capacity]);
	}
	const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator(&buffer[mSize]);
	}

	constexpr std::size_t size() const noexcept
	{
		return mCapacity;
	}

	void fill(const Element& value)
	{
		for (std::size_t i = 0u; i < mSize; ++i)
		{
			buffer[i] = value;
		}
	}

	std::size_t linearSearchFor(const Element& element)
	{
		for (std::size_t i = 0u; i < mSize; ++i)
		{
			if (this->get(i) == element) return i;
		}
		return i;
	}

	void sort()
	{
		std::sort(begin(), end());
	}

	void sort(bool(*comparator)(const Element&, const Element*))
	{
		std::sort(begin(), end(), comparator);
	}

	void sort(const std::size_t first, const std::size_t last)
	{
		std::sort(get(first), get(last));
	}

	void sort(const std::size_t first, const std::size_t last, bool(*comparator)(const Element&, const Element*))
	{
		std::sort(get(first), get(last), comparator);
	}

	void stableSort()
	{
		std::stable_sort(begin(), end());
	}

	void stableSort(bool(*comparator)(const Element&, const Element*))
	{
		std::stable_sort(begin(), end(), comparator);
	}

	void stableSort(const std::size_t first, const std::size_t last)
	{
		std::stable_sort(buffer[first], buffer[last]);
	}

	void stableSort(const std::size_t first, const std::size_t last, bool(*comparator)(const Element&, const Element*))
	{
		std::stable_sort(buffer[first], buffer[last], comparator);
	}

	void partialSort(const std::size_t middle)
	{
		std::partial_sort(begin(), buffer[middle], end());
	}

	void partialSort(const std::size_t middle, bool(*comparator)(const Element&, const Element*))
	{
		std::partial_sort(begin(), buffer[middle], end(), comparator);
	}

	void partialSort(const std::size_t first, const std::size_t middle, const std::size_t last)
	{
		std::partial_sort(buffer[first], buffer[middle], buffer[last]);
	}

	void partialSort(const std::size_t first, const std::size_t middle, const std::size_t last,
		bool(*comparator)(const Element&, const Element*))
	{
		std::partial_sort(buffer[first], buffer[middle], buffer[last], comparator);
	}

	void reverse()
	{
		std::reverse(begin(), end());
	}

	void reverse(std::size_t first, std::size_t last)
	{
		std::reverse(buffer[first], buffer[last]);
	}
};

template<class Element>
bool operator==(const ArrayBase<Element> lhs, const ArrayBase<Element> rhs)
{
	if (lhs.size() == rhs.size())
	{
		for (std::size_t i = 0u; i < lhs.size(); ++i)
		{
			if (lhs.get(i) != rhs.get(i)) return false;
		}
		return true;
	}
	return false;
}

template<class Element>
bool operator!=(const ArrayBase<Element> lhs, const ArrayBase<Element> rhs)
{
	return !(lhs == rhs);
}

template<class Element>
bool operator<(const ArrayBase<Element> lhs, const ArrayBase<Element> rhs)
{
	const std::size_t minSize = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
	for (std::size_t i = 0u; i < minSize; ++i)
	{
		if (lhs.get(i) < rhs.get(i)) return true;
		else if (lhs.get(i) > rhs.get(i)) return false;
	}
	return lhs.size() < rhs.size();
}

template<class Element>
bool operator<=(const ArrayBase<Element> lhs, const ArrayBase<Element> rhs)
{
	const std::size_t minSize = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
	for (std::size_t i = 0u; i < minSize; ++i)
	{
		if (lhs.get(i) < rhs.get(i)) return true;
		else if (lhs.get(i) > rhs.get(i)) return false;
	}
	return lhs.size() <= rhs.size();
}

template<class Element>
bool operator>(const ArrayBase<Element> lhs, const ArrayBase<Element> rhs)
{
	return !(lhs <= rhs);
}

template<class Element>
bool operator>=(const ArrayBase<Element> lhs, const ArrayBase<Element> rhs)
{
	return !(lhs < rhs);
}