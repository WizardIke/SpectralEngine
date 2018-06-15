#pragma once
#include <stdexcept>
#include <string>
#include <type_traits>
#include <algorithm>

template<class Element, std::size_t capacity>
class Array
{
	template<class Element, std::size_t capacity>
	class alignas(alignof(Element)) Buffer
	{
		alignas(alignof(Element)) unsigned char buffer[capacity * sizeof(Element)];
	public:
		Element & operator[](std::size_t i)
		{
			return (reinterpret_cast<Element*>(buffer))[i];
		}
		constexpr const Element& operator[](std::size_t i) const
		{
			return (reinterpret_cast<const Element*>(buffer))[i];
		}
	};

	Buffer<Element, capacity> buffer;
public:
	class DoNotInitialize {};

	typedef Element* iterator;
	using const_iterator = const Element*;
	typedef ::std::reverse_iterator<Element*> reverse_iterator;
	typedef ::std::reverse_iterator<const Element*> const_reverse_iterator;

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t, Element&)>>
	Array(Functor& initializer, typename std::enable_if<true, Return>::type* = nullptr)
	{
		std::size_t i;
		try
		{
			for (i = 0u; i < capacity; ++i)
			{
				initializer(i, buffer[i]);
			}
		}
		catch (...)
		{
			while (i != 0u)
			{
				--i;
				buffer[i].~Element();
			}
			throw;
		}
	}
	
	template<typename Functor, typename = typename std::result_of_t<Functor(std::size_t)> >
	Array(Functor& initializer)
	{
		std::size_t i;
		try
		{
			for (i = 0u; i < capacity; ++i)
			{
				new(&buffer[i]) Element(std::move(initializer(i)));
			}
		}
		catch (...)
		{
			while (i != 0u)
			{
				--i;
				buffer[i].~Element();
			}
			throw;
		}
	}

	Array(const Element(& elements)[capacity])
	{
		std::size_t i;
		try
		{
			for (i = 0u; i < capacity; ++i)
			{
				buffer[i] = elements[i];
			}
		}
		catch (...)
		{
			while (i != 0u)
			{
				--i;
				buffer[i].~Element();
			}
			throw;
		}
	}

	Array(Element(&&elements)[capacity])
	{
		std::size_t i;
		try
		{
			for (i = 0u; i < capacity; ++i)
			{
				buffer[i] = std::move(elements[i]);
			}
		}
		catch (...)
		{
			while (i != 0u)
			{
				--i;
				buffer[i].~Element();
			}
			throw;
		}
	}

	constexpr Array(DoNotInitialize) {}

	Array()
	{
		std::size_t i;
		try
		{
			for (i = 0u; i < capacity; ++i)
			{
				new(&buffer[i]) Element;
			}
		}
		catch (...)
		{
			while (i != 0u)
			{
				--i;
				buffer[i].~Element();
			}
			throw;
		}
	}

	Array(Array<Element, capacity>&& other)
	{
		for (std::size_t i = 0u; i < capacity; ++i)
		{
			new(&buffer[i]) Element(std::move(other.buffer[i]));
		}
	}

	~Array() 
	{
		std::size_t i = capacity;
		do
		{
			--i;
			buffer[i].~Element();
		} while (i != 0);
	}

	Element& at(const std::size_t pos)
	{
		using namespace std::string_literals;
	
		if (pos >= capacity)
		{
			std::string message = "Index "s;
			message += pos;
			message += "is out of range, the Array's size is "s;
			message += capacity;
			throw std::out_of_range(message);
		}
		return buffer[pos];
	}
	constexpr Element& at(const std::size_t pos) const
	{
		if (pos >= capacity)
		{
			using namespace std::string_literals;
			std::string message = "Index "s;
			message += pos;
			message += "is out of range, the Array's size is "s;
			message += capacity;
			throw std::out_of_range(message);
		}
		return buffer[pos];
	}
	Element& operator[](const std::size_t pos)
	{
		return buffer[pos];
	}
	constexpr Element& operator[](const std::size_t pos) const
	{
		return buffer[pos];
	}
	constexpr Element& get(const std::size_t pos) const
	{
		return *(&buffer[0] + pos);
	}
	Element& front() noexcept
	{
		return buffer[0];
	}
	Element& back() noexcept
	{
		return buffer[capacity - 1u];
	}
	Element* data() noexcept
	{
		return &buffer[0];
	}
	constexpr Element* data() const noexcept
	{
		return &buffer[0];
	}

	iterator begin() noexcept
	{
		return &buffer[0];
	}
	constexpr const_iterator begin() const noexcept
	{
		return &buffer[0];
	}
	constexpr const_iterator cbegin() const noexcept
	{
		return &buffer[0];
	}
	reverse_iterator rbegin() noexcept
	{
		return reverse_iterator(&buffer[0]);
	}
	const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator(&buffer[0]);
	}
	iterator end() noexcept
	{
		return &buffer[capacity];
	}
	constexpr const_iterator end() const noexcept
	{
		return &buffer[capacity];
	}
	constexpr const_iterator cend() const noexcept
	{
		return &buffer[capacity];
	}
	reverse_iterator  rend() noexcept
	{
		return reverse_iterator(&buffer[capacity]);
	}
	const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator(&buffer[capacity]);
	}

	constexpr bool empty() const noexcept
	{
		return false;
	}
	constexpr std::size_t size() const noexcept
	{
		return capacity;
	}
	constexpr std::size_t max_size() const noexcept
	{
		return capacity;
	}

	void swap(Array& other) //noexcept(noexcept(std::swap(declval<Element&>(), declval<Element&>())))
	{
		for (std::size_t i = 0u; i < capacity; ++i)
		{
			using std::swap;
			swap(buffer[i], other.buffer[i]);
		}
	}
};

template<typename Element, std::size_t capacity >
bool operator==(const Array<Element, capacity>& lhs, const Array<Element, capacity>& rhs)
{
	for (std::size_t i = 0u; i < capacity; ++i)
	{
		if (lhs.get(i) != rhs.get(i)) return false;
	}
	return true;
}

template< typename Element, std::size_t capacity >
bool operator!=(const Array<Element, capacity>& lhs, const Array<Element, capacity>& rhs)
{
	return !(lhs == rhs);
}

template< typename Element, std::size_t capacity >
bool operator<(const Array<Element, capacity>& lhs, const Array<Element, capacity>& rhs)
{
	for (std::size_t i = 0u; i < capacity; ++i)
	{
		if (lhs.get(i) < rhs.get(i)) return true;
		else if (lhs.get(i) > rhs.get(i)) return false;
	}
	return false;
}

template< typename Element, std::size_t capacity >
bool operator<=(const Array<Element, capacity>& lhs, const Array<Element, capacity>& rhs)
{
	for (std::size_t i = 0u; i < capacity; ++i)
	{
		if (lhs.get(i) < rhs.get(i)) return true;
		else if (lhs.get(i) > rhs.get(i)) return false;
	}
	return true;
}

template< typename Element, std::size_t capacity >
bool operator>(const Array<Element, capacity>& lhs, const Array<Element, capacity>& rhs)
{
	return !(lhs <= rhs);
}

template< typename Element, std::size_t capacity >
bool operator>=(const Array<Element, capacity>& lhs, const Array<Element, capacity>& rhs)
{
	return !(lhs < rhs);
}

template< std::size_t index, typename Element, std::size_t capacity >
constexpr Element& get(Array<Element, capacity>& a) noexcept
{
	using namespace std::string_literals;
	static_assert(index < capacity, "Index "s + index + " is out of range, the Array's size is "s + capacity);
	return a.buffer[index];
}

template< std::size_t index, typename Element, std::size_t capacity >
constexpr Element&& get(Array<Element, capacity>&& a) noexcept
{
	using namespace std::string_literals;
	static_assert(index < capacity, "Index "s + index + " is out of range, the Array's size is "s + capacity);
	return std::forward(a[index]);
}

template< std::size_t index, typename Element, std::size_t capacity >
constexpr const Element& get(const Array<Element, capacity>& a) noexcept
{
	using namespace std::string_literals;
	static_assert(index < capacity, "Index "s + index + " is out of range, the Array's size is "s + capacity);
	return a.buffer[index];
}

template< std::size_t index, typename Element, std::size_t capacity >
constexpr const Element&& get(const Array<Element, capacity>&& a) noexcept
{
	using namespace std::string_literals;
	static_assert(index < capacity, "Index "s + index + " is out of range, the Array's size is "s + capacity);
	return std::forward(a[index]);
}

template< typename Element, std::size_t capacity >
void swap(Array<Element, capacity>& lhs, Array<Element, capacity>& rhs)
{
	lhs.swap(rhs);
}

template< typename Element, std::size_t capacity >
class tuple_size : public std::integral_constant<std::size_t, capacity>
{ };

template<std::size_t index, typename Element, std::size_t capacity>
struct tuple_element
{
	using type = Element;
};