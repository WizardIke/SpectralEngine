#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits> //std::is_trivially_constructible_v, std::is_trivially_destructible_v
#include <limits> //std::numeric_limits
#include <array>
#include <new> //placement new, std::launder
#include <iterator> //std::reverse_iterator, std::random_access_iterator_tag
#undef max

template<class T, std::size_t cap, bool isTriviallyConstructible = std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>>
class FixedCapacityDynamicArray
{
public:
	using value_type = T;
	using size_type = std::conditional_t<cap <= std::numeric_limits<uint_least8_t>::max(), uint_least8_t,
		std::conditional_t<cap <= std::numeric_limits<uint_least16_t>::max(), uint_least16_t,
		std::conditional_t<cap <= std::numeric_limits<uint_least32_t>::max(), uint_least32_t,
		std::conditional_t<cap <= std::numeric_limits<uint_least64_t>::max(), uint_least64_t, std::size_t>>>>;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
private:
	//not a LegacyContiguousIterator because *(a + n) is not equivalent to *(std::addressof(*a) + n) for some dereferenceable iterator a and some dereferenceable iterator (a + n)
	//however, *(a + n) is equivalent to *(std::launder(std::addressof(*a) + n))
	class Iterator
	{
		unsigned char* data;

		friend class FixedCapacityDynamicArray<T, cap, isTriviallyConstructible>;
		constexpr Iterator(unsigned char* data1) noexcept : data{ data1 } {}
	public:
		using value_type = T;
		using pointer = value_type*;
		using reference = value_type&;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::random_access_iterator_tag;

		constexpr Iterator() noexcept : data{ nullptr } {}
		// Default Copy/Move Are Fine.
		// Default Destructor fine.

		constexpr reference operator*() const noexcept
		{
			return *std::launder(reinterpret_cast<value_type*>(data));
		}

		constexpr pointer operator->() const noexcept
		{
			return std::launder(reinterpret_cast<value_type*>(data));
		}

		constexpr reference operator[](difference_type i) const noexcept
		{
			return *std::launder(reinterpret_cast<value_type*>(data + i * sizeof(value_type)));
		}

		constexpr Iterator& operator++() noexcept
		{
			data += sizeof(value_type);
			return *this;
		}

		constexpr Iterator& operator--() noexcept
		{
			data -= sizeof(value_type);
			return *this;
		}

		constexpr Iterator operator++(int) noexcept
		{
			Iterator r(*this);
			data += sizeof(value_type);
			return r;
		}

		constexpr Iterator operator--(int) noexcept
		{
			Iterator r(*this);
			data -= sizeof(value_type);
			return r;
		}

		constexpr Iterator& operator+=(difference_type n) noexcept
		{
			data += n * sizeof(value_type);
			return *this;
		}

		constexpr Iterator& operator-=(difference_type n) noexcept
		{
			data -= n * sizeof(value_type);
			return *this;
		}

		friend constexpr Iterator operator+(const Iterator& it, difference_type n) noexcept
		{
			return { it.data + n * sizeof(value_type) };
		}

		friend constexpr Iterator operator-(const Iterator& it, difference_type n) noexcept
		{
			return { it.data - n * sizeof(value_type) };
		}

		constexpr difference_type operator-(const Iterator& r) const noexcept
		{
			return (data - r.data) / sizeof(value_type);
		}

		constexpr bool operator<(const Iterator& r) const noexcept { return data < r.data; }
		constexpr bool operator<=(const Iterator& r) const noexcept { return data <= r.data; }
		constexpr bool operator>(const Iterator& r) const noexcept { return data > r.data; }
		constexpr bool operator>=(const Iterator& r) const noexcept { return data >= r.data; }
		constexpr bool operator!=(const Iterator &r) const noexcept { return data != r.data; }
		constexpr bool operator==(const Iterator &r) const noexcept { return data == r.data; }
	};

	class ConstIterator
	{
		const unsigned char* data;

		friend class FixedCapacityDynamicArray<T, cap, isTriviallyConstructible>;
		constexpr ConstIterator(const unsigned char* data1) noexcept : data{ data1 } {}
	public:
		using value_type = T;
		using pointer = value_type * ;
		using reference = value_type & ;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::random_access_iterator_tag;

		constexpr ConstIterator() noexcept : data{ nullptr } {}
		constexpr ConstIterator(const Iterator& other) noexcept : data{ other.data } {}
		// Default Copy/Move Are Fine.
		// Default Destructor fine.

		constexpr const value_type& operator*() const noexcept
		{
			return *std::launder(reinterpret_cast<const value_type*>(data));
		}

		constexpr const value_type* operator->() const noexcept
		{
			return std::launder(reinterpret_cast<const value_type*>(data));
		}

		constexpr const value_type& operator[](difference_type i) const noexcept
		{
			return *std::launder(reinterpret_cast<const value_type*>(data + i * sizeof(value_type)));
		}

		constexpr ConstIterator& operator++() noexcept
		{
			data += sizeof(value_type);
			return *this;
		}

		constexpr ConstIterator& operator--() noexcept
		{
			data -= sizeof(value_type);
			return *this;
		}

		constexpr ConstIterator operator++(int) noexcept
		{
			ConstIterator r(*this);
			data += sizeof(value_type);
			return r;
		}

		constexpr ConstIterator operator--(int) noexcept
		{
			ConstIterator r(*this);
			data -= sizeof(value_type);
			return r;
		}

		constexpr ConstIterator& operator+=(difference_type n) noexcept
		{
			data += n * sizeof(value_type);
			return *this;
		}

		constexpr ConstIterator& operator-=(difference_type n) noexcept
		{
			data -= n * sizeof(value_type);
			return *this;
		}

		friend constexpr ConstIterator operator+(const ConstIterator& it, difference_type n) noexcept
		{
			return { it.data + n * sizeof(value_type) };
		}

		friend constexpr ConstIterator operator-(const ConstIterator& it, difference_type n) noexcept
		{
			return { it.data - n * sizeof(value_type) };
		}

		constexpr difference_type operator-(const ConstIterator& r) const noexcept
		{
			return (data - r.data) / sizeof(value_type);
		}

		constexpr bool operator<(const ConstIterator& r) const noexcept { return data < r.data; }
		constexpr bool operator<=(const ConstIterator& r) const noexcept { return data <= r.data; }
		constexpr bool operator>(const ConstIterator& r) const noexcept { return data > r.data; }
		constexpr bool operator>=(const ConstIterator& r) const noexcept { return data >= r.data; }
		constexpr bool operator!=(const ConstIterator &r) const noexcept { return data != r.data; }
		constexpr bool operator==(const ConstIterator &r) const noexcept { return data == r.data; }
	};
public:
	using iterator = Iterator;
	using const_iterator = ConstIterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
private:
#pragma warning(suppress: 4324) //warns about padding due to alignment
	alignas(alignof(value_type)) unsigned char data[sizeof(value_type) * cap];
	size_type mSize = static_cast<size_type>(0u);

	void removeAllData() noexcept
	{
		if (mSize != 0u)
		{
			auto sizeInBytes = mSize * sizeof(value_type);
			do
			{
				sizeInBytes -= sizeof(value_type);
				std::launder(reinterpret_cast<value_type*>(data + sizeInBytes))->~value_type();
			} while (sizeInBytes != 0u);
		}
	}
public:
	constexpr FixedCapacityDynamicArray() noexcept {}

	FixedCapacityDynamicArray(const FixedCapacityDynamicArray& other) : mSize{ other.mSize }
	{
		const auto sizeInBytes = mSize * sizeof(value_type);
		for (size_type i = static_cast<size_type>(0u); i != sizeInBytes; i += sizeof(value_type))
		{
			new(data + i) value_type{ *(std::launder(reinterpret_cast<const value_type*>(other.data + i))) };
		}
	}

	FixedCapacityDynamicArray(FixedCapacityDynamicArray&& other) : mSize{ other.mSize }
	{
		const auto sizeInBytes = mSize * sizeof(value_type);
		for (size_type i = static_cast<size_type>(0u); i != sizeInBytes; i += sizeof(value_type))
		{
			new(data + i) value_type{ std::move(*(std::launder(reinterpret_cast<value_type*>(other.data + i)))) };
		}
	}

	void operator=(const FixedCapacityDynamicArray& other)
	{
		removeAllData();
		mSize = other.mSize;
		const auto sizeInBytes = mSize * sizeof(value_type);
		for (size_type i = static_cast<size_type>(0u); i != sizeInBytes; i += sizeof(value_type))
		{
			new(data + i) value_type{ *(std::launder(reinterpret_cast<const value_type*>(other.data + i))) };
		}
	}

	void operator=(FixedCapacityDynamicArray&& other)
	{
		removeAllData();
		mSize = other.mSize;
		const auto sizeInBytes = mSize * sizeof(value_type);
		for (size_type i = static_cast<size_type>(0u); i != sizeInBytes; i += sizeof(value_type))
		{
			new(data + i) value_type{ std::move(*(std::launder(reinterpret_cast<value_type*>(other.data + i)))) };
		}
	}

	~FixedCapacityDynamicArray() noexcept
	{
		removeAllData();
	}

	void clear() noexcept
	{
		removeAllData();
		mSize = static_cast<size_type>(0u);
	}

	value_type& operator[](size_type i) noexcept
	{
		return *(std::launder(reinterpret_cast<value_type*>(data + i * sizeof(value_type))));
	}

	const value_type& operator[](size_type i) const noexcept
	{
		return *(std::launder(reinterpret_cast<const value_type*>(data + i * sizeof(value_type))));
	}

	void push_back(const value_type& value)
	{
		new(data + mSize * sizeof(value_type)) value_type{ value };
		++mSize;
	}

	void push_back(T&& value)
	{
		new(data + mSize * sizeof(value_type)) value_type{ std::move(value) };
		++mSize;
	}

	template<class... Args>
	void emplace_back(Args&&... args)
	{
		new(data + mSize * sizeof(value_type)) value_type{ std::forward<Args>(args)... };
	}

	void pop_back() noexcept
	{
		--mSize;
		auto* valuePtr = std::launder(reinterpret_cast<value_type*>(data + mSize * sizeof(value_type)));
		valuePtr->~value_type();
	}

	constexpr size_type size() const noexcept
	{
		return mSize;
	}

	constexpr static size_type capacity() noexcept
	{
		return static_cast<size_type>(cap);
	}

	constexpr iterator begin() noexcept
	{
		return { data };
	}

	constexpr const_iterator begin() const noexcept
	{
		return { data };
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return { data };
	}

	constexpr iterator end() noexcept
	{
		return { data + mSize };
	}

	constexpr const_iterator end() const noexcept
	{
		return { data + mSize };
	}

	constexpr const_iterator cend() const noexcept
	{
		return { data + mSize };
	}

	constexpr reverse_iterator rbegin() noexcept
	{
		return reverse_iterator{ end() };
	}

	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator{ end() };
	}

	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator{ end() };
	}

	constexpr reverse_iterator rend() noexcept
	{
		return reverse_iterator{ begin() };
	}

	constexpr const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator{ begin() };
	}

	constexpr const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator{ begin() };
	}
};

template<class T, std::size_t cap>
class FixedCapacityDynamicArray<T, cap, true>
{
public:
	using value_type = T;
	using size_type = std::conditional_t<cap <= std::numeric_limits<uint_least8_t>::max(), uint_least8_t,
		std::conditional_t<cap <= std::numeric_limits<uint_least16_t>::max(), uint_least16_t,
		std::conditional_t<cap <= std::numeric_limits<uint_least32_t>::max(), uint_least32_t,
		std::conditional_t<cap <= std::numeric_limits<uint_least64_t>::max(), uint_least64_t, std::size_t>>>>;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
private:
	std::array<value_type, cap> data;
	size_type mSize = static_cast<size_type>(0u);
public:
	constexpr FixedCapacityDynamicArray() noexcept {}

	FixedCapacityDynamicArray(const FixedCapacityDynamicArray& other) : mSize{ other.mSize }
	{
		const auto size = mSize;
		for (size_type i = static_cast<size_type>(0u); i != size; ++i)
		{
			data[i] = other.data[i];
		}
	}

	FixedCapacityDynamicArray(FixedCapacityDynamicArray&& other) : mSize{ other.mSize }
	{
		const auto size = mSize;
		for (size_type i = static_cast<size_type>(0u); i != size; ++i)
		{
			data[i] = std::move(other.data[i]);
		}
	}

	void operator=(const FixedCapacityDynamicArray& other)
	{
		mSize = other.mSize;
		const auto size = mSize;
		for (size_type i = static_cast<size_type>(0u); i != size; ++i)
		{
			data[i] = other.data[i];
		}
	}

	void operator=(FixedCapacityDynamicArray&& other)
	{
		mSize = other.mSize;
		const auto size = mSize;
		for (size_type i = static_cast<size_type>(0u); i != size; ++i)
		{
			data[i] = std::move(other.data[i]);
		}
	}

	constexpr void clear() noexcept
	{
		mSize = static_cast<size_type>(0u);
	}

	constexpr value_type& operator[](size_type i) noexcept
	{
		return data[i];
	}

	constexpr const value_type& operator[](size_type i) const noexcept
	{
		return data[i];
	}

	constexpr void push_back(const value_type& value)
	{
		data[mSize] = value;
		++mSize;
	}

	constexpr void push_back(value_type&& value)
	{
		data[mSize] = std::move(value);
		++mSize;
	}

	template<class... Args>
	constexpr void emplace_back(Args&&... args)
	{
		data[mSize] = { std::forward<Args>(args)... };
		++mSize;
	}

	template<class Arg>
	constexpr void emplace_back(Arg&& arg)
	{
		data[mSize] = std::forward<Arg>(arg);
		++mSize;
	}

	constexpr void pop_back() noexcept
	{
		--mSize;
	}

	constexpr size_type size() const noexcept
	{
		return mSize;
	}

	constexpr static size_type capacity() noexcept
	{
		return static_cast<size_type>(cap);
	}

	constexpr iterator begin() noexcept
	{
		return data.data();
	}

	constexpr const_iterator begin() const noexcept
	{
		return data.data();
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return data.data();
	}

	constexpr iterator end() noexcept
	{
		return data.data() + mSize;
	}

	constexpr const_iterator end() const noexcept
	{
		return data.data() + mSize;
	}

	constexpr const_iterator cend() const noexcept
	{
		return data.data() + mSize;
	}

	constexpr reverse_iterator rbegin() noexcept
	{
		return reverse_iterator{ end() };
	}

	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator{ end() };
	}

	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator{ end() };
	}

	constexpr reverse_iterator rend() noexcept
	{
		return reverse_iterator{ begin() };
	}

	constexpr const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator{ begin() };
	}

	constexpr const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator{ begin() };
	}
};