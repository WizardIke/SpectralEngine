#pragma once
#include <initializer_list>
#include <memory>

template<class Element, class Allocator = std::allocator<Element>>
class DynamicArray : private Allocator
{
	Element* buffer;
	Element* mEnd;
public:
	using allocator_type = Allocator;
	using size_type = std::size_t;
	using value_type = Element;
	using iterator = Element*;
	using const_iterator = const Element*;
	using reverse_iterator = ::std::reverse_iterator<iterator>;
	using const_reverse_iterator = ::std::reverse_iterator<const_iterator>;
	using pointer = value_type*;
	using reference = value_type&;
	using const_pointer = const value_type*;
	using const_reference = const value_type&;

	class DoNotInitialize {};

	struct Size
	{
		std::size_t size;
		operator std::size_t&()
		{
			return size;
		}
	};

	DynamicArray(DynamicArray&& over)
	{
		buffer = other.buffer;
		mEnd = other.mEnd;
		other.mEnd = buffer;
	}

	DynamicArray(const Size size, DoNotInitialize)
	{
		buffer = this->allocate(size.size);
		mEnd = buffer + size.size;
	}

	DynamicArray(const Size size)
	{
		buffer = this->allocate(size.size);
		mEnd = buffer + size.size;

		std::size_t i;
		try
		{
			for (i = 0u; i != size.size; ++i)
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

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t)> >
	DynamicArray(const Size size, Functor& initializer, typename std::enable_if<true, Return>::type* = nullptr) :
	{
		buffer = this->allocate(size.size);
		mEnd = buffer + size.size;

		std::size_t i;
		try
		{
			for (i = 0u; i != size.size; ++i)
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
	
	template<typename Functor, typename = typename std::result_of_t<Functor(std::size_t, Element&)> >
	DynamicArray(const Size size, Functor& initializer)
	{
		buffer = this->allocate(size.size);
		mEnd = buffer + size.size;

		std::size_t i;
		try
		{
			for (i = 0u; i != size.size; ++i)
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

	DynamicArray(const std::initializer_list<Element> initializer)
	{
		const std::size_t size = initializer.size();
		buffer = this->allocate(size);
		mEnd = buffer + size;
		
		std::size_t i;
		try
		{
			decltype(initializer.begin()) j;
			for (i = 0u, j = initializer.begin(); j != initializer.end(); j++, ++i)
			{
				new(&buffer[i]) Element(*j);
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

	DynamicArray(std::initializer_list<Element>&& initializer)
	{
		const std::size_t size = initializer.size();
		buffer = this->allocate(size);
		mEnd = buffer + size;

		std::size_t i;
		try
		{
			decltype(initializer.begin()) j;
			for (i = 0u, j = initializer.begin(); j != initializer.end(); j++, ++i)
			{
				new(&buffer[i]) Element(std::move(*j));
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

	DynamicArray(const Size size, const std::initializer_list<Element> initializer)
	{
		const std::size_t size = initializer.size() > size.size ? initializer.size() : size.size;
		buffer = this->allocate(size);
		mEnd = buffer + size;

		std::size_t i;
		try
		{
			decltype(initializer.begin()) j;
			for (i = 0u, j = initializer.begin();; j++, ++i)
			{
				if (i == size) break;
				if (j == initializer.end())
				{
					for (; i < size.size; ++i)
					{
						new(&buffer[i]) Element;
					}
					break;
				}
				new(&buffer[i]) Element(*j);
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

	DynamicArray(const Size size, std::initializer_list<Element>&& initializer)
	{
		const std::size_t size = initializer.size() > size.size ? initializer.size() : size.size;
		buffer = this->allocate(size);
		mEnd = buffer + size;

		std::size_t i;
		try
		{
			decltype(initializer.begin()) j;
			for (i = 0u, j = initializer.begin();; j++, ++i)
			{
				if (i == size) break;
				if (j == initializer.end())
				{
					for (; i < size.size; ++i)
					{
						new(&buffer[i]) Element;
					}
					break;
				}
				new(&buffer[i]) Element(std::move(*j));
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

	~DynamicArray()
	{
		if (buffer != mEnd)
		{
			size_type size = mEnd - buffer;
			do
			{
				--mEnd;
				mEnd->~Element();
			} while (buffer != mEnd);
			this->deallocate(buffer, size);
		}
	}

	constexpr bool empty() const noexcept
	{
		return buffer == mEnd;
	}

	void swap(DynamicArray& other) // noexcept(noexcept(std::swap(declval<ElementType&>(), declval<ElementType&>())))
	{
		Element* temp;
		temp = this->mEnd;
		this->mEnd = other.mEnd;
		other.mEnd = temp;

		temo = this->temp;
		this->buffer = other.buffer;
		other.buffer = temp;
	}

	const DynamicArray& operator=(DynamicArray&& other)
	{
		this->~DynamicArray();
		new(this) DynamicArray(std::move(other));
		return *this;
	}

	constexpr std::size_t max_size() const noexcept
	{
		return size();
	}

	Element& at(const std::size_t pos)
	{
		auto element = buffer + pos;
		if (element >= mEnd)
		{
			using namespace std::string_literals;
			std::string message = "Index "s;
			message += pos;
			message += "is out of range, the Array's size is "s;
			message += size();
			throw std::out_of_range(message);
		}
		return *element;
	}
	constexpr Element& at(const std::size_t pos) const
	{
		const auto element = buffer + pos;
		if (element >= mEnd)
		{
			using namespace std::string_literals;
			std::string message = "Index "s;
			message += pos;
			message += "is out of range, the Array's size is "s;
			message += size();
			throw std::out_of_range(message);
		}
		return *element;
	}
	Element& operator[](const std::size_t pos)
	{
		return buffer[pos];
	}
	const Element& operator[](const std::size_t pos) const
	{
		return buffer[pos];
	}
	Element& front()
	{
		return buffer[0];
	}
	Element& back()
	{
		return *(mEnd - 1u);
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
		return mEnd;
	}
	const_iterator end() const noexcept
	{
		return mEnd;
	}
	constexpr iterator cend() const noexcept
	{
		return mEnd;
	}
	reverse_iterator  rend() noexcept
	{
		return reverse_iterator(mEnd);
	}
	const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator(mEnd);
	}

	constexpr std::size_t size() const noexcept
	{
		return mEnd - buffer;
	}
};

template<class Element, class Allocator>
void swap(DynamicArray<Element, Allocator>& lhs, DynamicArray<Element, Allocator>& rhs)
{
	lhs.swap(rhs);
}

template<class Element, class Allocator>
bool operator==(const DynamicArray<Element, Allocator>& lhs, const DynamicArray<Element, Allocator>& rhs)
{
	if (lhs.size() == rhs.size())
	{
		for (std::size_t i = 0u; i != lhs.size(); ++i)
		{
			if (lhs[i] != rhs[i]) return false;
		}
		return true;
	}
	return false;
}

template<class Element, class Allocator>
bool operator!=(const DynamicArray<Element, Allocator>& lhs, const DynamicArray<Element, Allocator>& rhs)
{
	return !(lhs == rhs);
}

template<class Element, class Allocator>
bool operator<(const DynamicArray<Element, Allocator>& lhs, const DynamicArray<Element, Allocator>& rhs)
{
	const std::size_t minSize = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
	for (std::size_t i = 0u; i != minSize; ++i)
	{
		if (lhs[i] < rhs[i]) return true;
		else if (lhs[i] > rhs[i]) return false;
	}
	return lhs.size() < rhs.size();
}

template<class Element, class Allocator>
bool operator>(const DynamicArray<Element, Allocator>& lhs, const DynamicArray<Element, Allocator>& rhs)
{
	return rhs < lhs;
}

template<class Element, class Allocator>
bool operator<=(const DynamicArray<Element, Allocator>& lhs, const DynamicArray<Element, Allocator>& rhs)
{
	return !(lhs > rhs);
}

template<class Element, class Allocator>
bool operator>=(const DynamicArray<Element, Allocator>& lhs, const DynamicArray<Element, Allocator>& rhs)
{
	return !(lhs < rhs);
}