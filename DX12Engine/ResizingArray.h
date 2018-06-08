#pragma once
#include <cstddef>
#include <memory>
#undef min
#undef max

template<class Element, class Allocator = std::allocator<Element>>
class ResizingArray : private Allocator
{
	using traits = std::allocator_traits<Allocator>;
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

	struct Size
	{
		std::size_t size;
		operator std::size_t&()
		{
			return size;
		}
	};
private:
	Element* mCapacityEnd;
	Element* buffer;
	Element* mEnd;
	
	void resizeNoChecks()
	{
		size_type newSize = mCapacityEnd - buffer;
		newSize = newSize + newSize / 2u + 1u;// try to grow by 50%

		resizeToKnownSizeNoChechs(newSize);
	}

	void resizeNoChecks(std::size_t minCapacity)
	{
		size_type newSize = mCapacityEnd - buffer;
		newSize = (newSize + newSize / 2u) > minCapacity ? (newSize + newSize / 2u) : minCapacity;// try to grow by 50%

		resizeToKnownSizeNoChechs(newSize);
	}
	void resizeToKnownSizeNoChechs(size_type newSize)
	{
		Element*const temp = this->allocate(newSize);
		size_type size = mEnd - buffer;
		if (size != 0u)
		{
			size_t i = size;
			do
			{
				--i;
				new(&temp[i]) Element(std::move(buffer[i]));
				buffer[i].~Element();
			} while (i != 0u);
			this->deallocate(buffer, size);
		}
		buffer = temp;
		mEnd = buffer + size;
		mCapacityEnd = buffer + newSize;
	}
public:
	ResizingArray() noexcept : buffer(nullptr), mEnd(nullptr), mCapacityEnd(nullptr) {}

	ResizingArray(ResizingArray&& other) : buffer(other.buffer), mEnd(other.mEnd), mCapacityEnd(other.mCapacityEnd)
	{
		other.buffer = nullptr;
		other.mEnd = nullptr;
	}

	ResizingArray(const Size capacity)
	{
		buffer = this->allocate(capacity.size);
		mEnd = buffer;
		mCapacityEnd = buffer + capacity.size;
	}

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t)> >
	ResizingArray(const Size capacity, Functor& initializer, typename std::enable_if<true, Return>::type* = nullptr)
	{
		buffer = this->allocate(capacity.size);
		mEnd = buffer + capacity.size;
		mCapacityEnd = mEnd;

		std::size_t i;
		try
		{
			for (i = 0u; i < capacity.size; ++i)
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

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t, Element&)> >
	ResizingArray(const Size capacity, Functor& initializer)
	{
		buffer = this->allocate(capacity.size);
		mEnd = buffer + capacity.size;
		mCapacityEnd = mEnd;

		std::size_t i;
		try
		{
			for (i = 0u; i < capacity.size; ++i)
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

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t)> >
	ResizingArray(const Size size, const Size capacity, Functor& initializer, typename std::enable_if<true, Return>::type* = nullptr)
	{
		buffer = this->allocate(capacity.size);
		mEnd = buffer + size;
		mCapacityEnd = buffer + capacity.size;

		std::size_t i;
		try
		{
			for (i = 0u; i < size.size; ++i)
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

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t, Element&)> >
	ResizingArray(const Size size, const Size capacity, Functor& initializer)
	{
		buffer = this->allocate(capacity.size);
		mEnd = buffer + size;
		mCapacityEnd = buffer + capacity.size;

		std::size_t i;
		try
		{
			for (i = 0u; i < size.size; ++i)
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

	ResizingArray(const std::initializer_list<Element> initializer)
	{
		const std::size_t size = initializer.size();
		buffer = this->allocate(size);
		mEnd = buffer + size;
		mCapacityEnd = mEnd;

		auto current = buffer;
		try
		{
			auto j = initializer.begin();
			for (; j != initializer.end(); ++j, ++current)
			{
				new(current) Element(*j);
			}
		}
		catch (...)
		{
			while (current != buffer)
			{
				--current;
				current->~Element();
			}
			throw;
		}
	}

	ResizingArray(const Size capacity, const std::initializer_list<Element> initializer)
	{
		const std::size_t size = initializer.size();
		buffer = this->allocate(capacity);
		mEnd = buffer + size;
		mCapacityEnd = buffer + capacity;

		auto current = buffer;
		try
		{
			auto j = initializer.begin();
			for (; j != initializer.end(); ++j, ++current)
			{
				new(current) Element(*j);
			}
		}
		catch (...)
		{
			while (current != buffer)
			{
				--current;
				current->~Element();
			}
			throw;
		}
	}

	~ResizingArray()
	{
		if (buffer != mEnd)
		{
			do
			{
				--mEnd;
				mEnd->~Element();
			} while (buffer != mEnd);
			this->deallocate(buffer, mCapacityEnd - buffer);
		}
	}

	bool empty() const noexcept
	{
		return buffer == mEnd;
	}

	void swap(ResizingArray& other) noexcept
	{
		Element* temp;

		temp = this->mEnd;
		this->mEnd = other.mEnd;
		other.mEnd = temp;

		temp = this->buffer;
		this->buffer = other.buffer;
		other.buffer = temp;

		temp = this->mCapacityEnd;
		this->mCapacityEnd = other.mCapacityEnd;
		other.mCapacityEnd = temp;
	}

	const ResizingArray& operator=(ResizingArray&& other)
	{
		this->~ResizingArray();
		new(this) ResizingArray(std::move(other));
		return *this;
	}

	constexpr static std::size_t max_size() noexcept
	{
		return std::numeric_limits<std::size_t>::max();
	}

	void assign(std::size_t count, const Element& value)
	{
		while (buffer != mEnd)
		{
			--mEnd;
			mEnd.~Element();
		} 
		size_type capacity = mCapacityEnd - buffer;
		if (count > capacity)
		{
			this->deallocate(buffer, capacity);
			buffer = this->allocate(count);
			mCapacityEnd = buffer + count;
		}
		mEnd = buffer + count;
		for (auto& value2 : *this)
		{
			new(&value2) Element(value);
		}
	}

	template< class InputIt >
	void assign(InputIt first, InputIt last)
	{
		std::size_t count = std::distance(first, last);

		while (buffer != mEnd)
		{
			--mEnd;
			mEnd.~Element();
		}
		size_type capacity = mCapacityEnd - buffer;
		if (count > capacity)
		{
			this->deallocate(buffer, capacity);
			buffer = this->allocate(count);
			mCapacityEnd = buffer + count;
		}
		mEnd = buffer + count;
		for (auto& value2 : *this)
		{
			new(&value2) Element(value);
		}
		for (auto current = buffer; first != last; ++first, ++current)
		{
			new(current) Element(*first);
		}
	}
	
	void assign(std::initializer_list<Element> ilist)
	{
		this->assign(ilist.begin(), ilist.end());
	}

	void reserve(std::size_t new_cap)
	{
		if (new_cap <= mCapacity) return;
		resizeToKnownSizeNoChechs(new_cap);
	}

	std::size_t capacity() const noexcept
	{
		return mCapacityEnd - buffer;
	}

	void shrink_to_fit()
	{
		if (mCapacityEnd > mEnd)
		{
			resizeToKnownSizeNoChechs(mEnd - buffer);
		}
	}

	void clear() noexcept
	{
		while (buffer != mEnd)
		{
			--mEnd;
			mEnd->~Element();
		}
	}

	iterator insert(iterator pos, const Element& value)
	{
		if (pos >= mCapacityEnd)
		{
			std::size_t new_cap = pos - buffer + 1u;
			size_type index = pos - buffer;
			resizeToKnownSizeNoChechs(new_cap);
			pos = buffer + index;
		}
		else if (mCapacityEnd == mEnd)
		{
			size_type index = pos - buffer;
			resizeNoChecks();
			pos = buffer + index;
		}

		if(pos >= mEnd)
		{
			for (; mEnd != pos; ++mEnd)
			{
				new(mEnd) Element;
			}
			*pos = value;
			++mEnd;
		}
		else
		{
			auto current = mEnd;
			new(current) Element(std::move(current - 1u));
			--current;
			while (current != pos)
			{
				current = std::move(current - 1u);
			}
			*pos = value;
			++mEnd;
		}
		return pos;
	}

	iterator insert(iterator pos, Element&& value)
	{
		if (pos >= mCapacityEnd)
		{
			std::size_t new_cap = pos - buffer + 1u;
			size_type index = pos - buffer;
			resizeToKnownSizeNoChechs(new_cap);
			pos = buffer + index;
		}
		else if (mCapacityEnd == mEnd)
		{
			size_type index = pos - buffer;
			resizeNoChecks();
			pos = buffer + index;
		}

		if (pos >= mEnd)
		{
			for (; mEnd != pos; ++mEnd)
			{
				new(mEnd) Element;
			}
			*pos = std::move(value);
			++mEnd;
		}
		else
		{
			auto current = mEnd;
			new(current) Element(std::move(current - 1u));
			--current;
			while (current != pos)
			{
				current = std::move(current - 1u);
			}
			*pos = std::move(value);
			++mEnd;
		}
		return pos;
	}

	iterator insert(iterator pos, size_t count, const Element& value)
	{
		if (count == 0u) return pos;

		if (pos + count > mCapacityEnd)
		{

			size_type index = pos - buffer;
			std::size_t new_cap = index + count;
			resizeToKnownSizeNoChechs(new_cap);
			pos = buffer + index;
		}
		else if (mEnd + count > mCapacityEnd)
		{
			size_type index = pos - buffer;
			resizeNoChecks();
			pos = buffer + index;
		}

		if (pos >= mEnd)
		{
			for (; mEnd != pos; ++mEnd)
			{
				new(mEnd) Element;
			}
			const auto newEnd = mEnd + count;
			for (; mEnd != newEnd; ++mEnd)
			{
				new(mEnd) Element(value);
			}
		}
		else
		{
			size_type numerToMove = mEnd - pos;
			auto current = mEnd;
			do
			{
				--current;
				new(current + numerToMove) Element(std::move(*current));
				current->~Element();
			} while (current != pos);
			mEnd += count;

			const auto newEnd = pos + count;
			for (auto current = pos; current != newEnd; ++current)
			{
				new(current) Element(value);
			}
		}

		return pos;
	}

	template< typename InputIt >
	iterator insert(iterator pos, InputIt first, InputIt last)
	{
		if (first == last) return pos;
		size_t count = std::distance(first, last);

		if (pos + count > mCapacityEnd)
		{
			
			size_type index = pos - buffer;
			std::size_t new_cap = index + count;
			resizeToKnownSizeNoChechs(new_cap);
			pos = buffer + index;
		}
		else if (mEnd + count > mCapacityEnd)
		{
			size_type index = pos - buffer;
			resizeNoChecks();
			pos = buffer + index;
		}

		if (pos >= mEnd)
		{
			for (; mEnd != pos; ++mEnd)
			{
				new(mEnd) Element;
			}
			for (; first != last; ++first, ++mEnd)
			{
				new(mEnd) Element(*first);
			}
		}
		else
		{
			size_type numerToMove = mEnd - pos;
			auto current = mEnd;
			do
			{
				--current;
				new(current + numerToMove) Element(std::move(*current));
				current->~Element();
			} while (current != pos);
			mEnd += count;

			for (auto current = pos; first != last; ++first, ++current)
			{
				new(current) Element(*first);
			}
		}

		return pos;
	}

	iterator insert(iterator pos, std::initializer_list<Element> ilist)
	{
		return insert(pos, ilist.begin(), ilist.end());
	}

	template<class... Args>
	iterator emplace(iterator pos, Args&&... args)
	{
		if (pos >= mCapacityEnd)
		{
			std::size_t new_cap = pos - buffer + 1u;
			size_type index = pos - buffer;
			resizeToKnownSizeNoChechs(new_cap);
			pos = buffer + index;
		}
		else if (mCapacityEnd == mEnd)
		{
			size_type index = pos - buffer;
			resizeNoChecks();
			pos = buffer + index;
		}

		if (pos >= mEnd)
		{
			for (; mEnd != pos; ++mEnd)
			{
				new(mEnd) Element;
			}
			pos->~Element();
			new(pos) Element(std::forward<Args>(args)...);
			++mEnd;
		}
		else
		{
			auto current = mEnd;
			new(current) Element(std::move(current - 1u));
			--current;
			while (current != pos)
			{
				current = std::move(current - 1u);
			}
			pos->~Element();
			new(pos) Element(std::forward<Args>(args)...);
			++mEnd;
		}
		return pos;
	}

	iterator erase(iterator pos)
	{
		--mEnd;
		for (auto current = pos; current != mEnd; ++current)
		{
			current = std::move(current + 1u);
		}
		mEnd.~Element();
		return pos;
	}

	iterator erase(iterator first, iterator last)
	{
		size_t numElements = 0u;
		for (; first != last; ++first, ++numElements)
		{
			first->~Element();
		}
		const auto end = mEnd;
		for (; last != end; ++last)
		{
			auto pos = last - numElements;
			new(pos) Element(std::move(*last));
			last->~Element();
		}
		mEnd -= numElements;
	}

	void push_back(const Element& value)
	{
		if (mEnd == mCapacityEnd) resizeNoChecks();
		new(mEnd) Element(value);
		++mEnd;
	}

	void push_back(Element&& value)
	{
		if (mEnd == mCapacityEnd) resizeNoChecks();
		new(mEnd) Element(std::move(value));
		++mEnd;
	}

	template<typename... Args>
	reference emplace_back(Args&&... args)
	{
		if (mEnd == mCapacityEnd) resizeNoChecks();
		new(mEnd) Element(std::forward<Args>(args)...);
		++mEnd;
		return *(mEnd - 1u);
	}

	void pop_back()
	{
		--mEnd;
		mEnd->~Element();
	}

	void resize(size_t count)
	{
		auto newEnd = buffer + count;
		if (newEnd < mEnd)
		{
			do
			{
				--mEnd;
				mEnd->~Element();
			} while (newEnd < mEnd);
		}
		else
		{
			if (newEnd > mCapacityEnd)
			{
				resizeToKnownSizeNoChechs(count);
				newEnd = buffer + count;
			}
			while (mEnd != newEnd)
			{
				new(mEnd) Element;
			}
		}
	}

	void resize(size_t count, const Element& value)
	{
		auto newEnd = buffer + count;
		if (newEnd < mEnd)
		{
			do
			{
				--mEnd;
				mEnd->~Element();
			} while (newEnd < mEnd);
		}
		else
		{
			if (newEnd > mCapacityEnd)
			{
				resizeToKnownSizeNoChechs(count);
				newEnd = buffer + count;
			}
			while (mEnd != newEnd)
			{
				new(mEnd) Element(value);
			}
		}
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

template<typename Element>
void swap(ResizingArray<Element>& lhs, ResizingArray<Element>& rhs)
{
	lhs.swap(rhs);
}

template<class Element>
bool operator==(const ResizingArray<Element>& lhs, const ResizingArray<Element>& rhs)
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

template<class Element>
bool operator!=(const ResizingArray<Element>& lhs, const ResizingArray<Element>& rhs)
{
	return !(lhs == rhs);
}

template<class Element>
bool operator<(const ResizingArray<Element>& lhs, const ResizingArray<Element>& rhs)
{
	const std::size_t minSize = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
	for (std::size_t i = 0u; i != minSize; ++i)
	{
		if (lhs[i] < rhs[i]) return true;
		else if (lhs[i] > rhs[i]) return false;
	}
	return lhs.size() < rhs.size();
}

template<class Element>
bool operator>(const ResizingArray<Element>& lhs, const ResizingArray<Element>& rhs)
{
	return rhs < lhs;
}

template<class Element>
bool operator<=(const ResizingArray<Element>& lhs, const ResizingArray<Element>& rhs)
{
	return !(lhs > rhs);
}

template<class Element>
bool operator>=(const ResizingArray<Element>& lhs, const ResizingArray<Element>& rhs)
{
	return !(lhs < rhs);
}