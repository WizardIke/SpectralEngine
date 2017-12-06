#pragma once
#include "ArrayBase.h"
#include "ArraySize.h"
#include <cstddef>

template<class Element, class Alloc = allocator<Element> >
class ResizingArray : public ArrayBase<Element>, private Alloc
{
public:
	using value_type = Element;
	using allocator_type = Alloc;
	using size_type = typename traits::size_type;
private:
	size_type size;

	using traits = std::allocator_traits<allocator_type>;
	using pointer = typename traits::pointer;

	void resizeNoChecks()
	{
		size_type newSize = mCapacity;
		newSize = newSize + newSize / 2u + 1u > max_size() ? max_size() : newSize + newSize / 2u + 1u;// try to grow by 50%

		resizeToKnownSizeNoChechs(newSize);
	}

	void resizeNoChecks(std::size_t minCapacity)
	{
		size_type newSize = mCapacity;
		newSize = newSize + newSize / 2u > max_size() ? max_size() :
			(newSize + newSize / 2u > minCapacity ? newSize + newSize / 2u : minCapacity);// try to grow by 50%

		resizeToKnownSizeNoChechs(newSize);
	}
	void resizeToKnownSizeNoChechs(size_type newSize)
	{
		Element*const temp = reinterpret_cast<Element*const>(new Memory<sizeof(Element), alignof(Element)>[newSize]);
		if (size != 0u)
		{
			size_t i = size;
			do
			{
				new(&temp[i]) Element(std::move(buffer[i]));
				buffer[i].~Element();
				--i;
			} while (i != 0u);
		}
		delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
		buffer = temp;
		mCapacity = newSize;
	}
public:
	ResizingArray() noexcept : ArrayBase<Element>(nullptr, 0u), size(0u) {}

	ResizingArray(const ArraySize capacity) : ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element),
		alignof(Element)>[capacity.size]), capacity.size), size(0) {}

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t)> >
	ResizingArray(const ArraySize capacity, Functor& initializer, typename std::enable_if<true, Return>::type* = nullptr) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[capacity.size]),
			capacity.size), size(capacity.size)
	{
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t, Element&)> >
	ResizingArray(const ArraySize capacity, Functor& initializer) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[capacity.size]),
			capacity.size), size(capacity.size)
	{
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t)> >
	ResizingArray(const ArraySize size, const ArraySize capacity, Functor& initializer, typename std::enable_if<true, Return>::type* = nullptr) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[capacity.size]),
			capacity.size), size(size.size)
	{
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	template<typename Functor, typename Return = typename std::result_of_t<Functor(std::size_t, Element&)> >
	ResizingArray(const ArraySize size, const ArraySize capacity, Functor& initializer) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[capacity.size]),
			capacity.size), size(size.size)
	{
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	ResizingArray(const std::initializer_list<Element> initializer) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[initializer.size()]),
			initializer.size()), size(initializer.size())
	{
		const std::size_t size = initializer.size();
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	ResizingArray(std::initializer_list<Element>&& initializer) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[initializer.size()]),
			initializer.size()), size(initializer.size())
	{
		const std::size_t size = initializer.size();
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	ResizingArray(const ArraySize capacity, const std::initializer_list<Element> initializer) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[capacity.size]),
			capacity.size), size(initializer.size())
	{
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	ResizingArray(const ArraySize capacity, std::initializer_list<Element>&& initializer) :
		ArrayBase<Element>(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[capacity.size]),
			capacity.size), size(initializer.size())
	{
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
			if (i != 0u)
			{
				do
				{
					buffer[i].~Element();
					--i;
				} while (i != 0u);
			}
			throw;
		}
	}

	~ResizingArray()
	{
		if (size != 0u)
		{
			auto i = size;
			do
			{
				buffer[i].~Element();
				--i;
			} while (i != 0u);
		}
		delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
	}

	bool empty() const noexcept
	{
		return size == 0;
	}

	void swap(ResizingArray& other) noexcept
	{
		union
		{
			std::size_t size;
			Element* buffer;
			std::size_t capacity;
		};
		size = this->size;
		this->size = other.size;
		other.size = size;

		buffer = this->buffer;
		this->buffer = other.buffer;
		other.buffer = buffer;

		capacity = this->mCapacity;
		this->mCapacity = other.mCapacity;
		other.mCapacity = capacity;
	}

	const ResizingArray& operator=(ResizingArray&& other)
	{
		if (size != 0u)
		{
			auto i = size;
			do
			{
				buffer[i].~Element();
				--i;
			} while (i != 0u);
		}
		delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
		mCapacity = other.mCapacity;
		size = other.size;
		buffer = other.buffer;
		other.buffer = nullptr;
		return *this;
	}

	constexpr static std::size_t max_size() noexcept
	{
		return std::numeric_limits<std::size_t>::max();
	}

	void assign(std::size_t count, const Element& value)
	{
		if (size)
		{
			auto i = size;
			do
			{
				buffer[i].~Element();
				--i;
			} while (i);
		}
		if (count > mCapacity)
		{
			delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
			buffer = reinterpret_cast<Element*const>(new Memory<sizeof(Element), alignof(Element)>[count]);
			mCapacity = count;
		}
		size = count;
		if (count)
		{
			do
			{
				--count;
				new(&buffer[count]) Element(value);
			} while (count);
		}
	}

	template< class InputIt >
	void assign(InputIt first, InputIt last)
	{
		std::size_t length = 0u;
		for (auto begin = first; begin != last; ++begin, ++length);

		if (size)
		{
			auto i = size;
			do
			{
				buffer[i].~Element();
				--i;
			} while (i);
		}

		if (length > mCapacity)
		{
			delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
			buffer = reinterpret_cast<Element*const>(new Memory<sizeof(Element), alignof(Element)>[length]);
			mCapacity = length;
		}
		size = length;
		for (std::size_t i = 0u; first != last; ++first, ++i)
		{
			new(&buffer[i]) Element(*first);
		}
	}
	
	void assign(std::initializer_list<Element> ilist)
	{
		std::size_t listSize = ilist.size();

		if (size)
		{
			auto i = size;
			do
			{
				buffer[i].~Element();
				--i;
			} while (i);
		}

		if (listSize > mCapacity)
		{
			delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
			buffer = reinterpret_cast<Element*const>(new Memory<sizeof(Element), alignof(Element)>[listSize]);
			mCapacity = listSize;
		}
		size = listSize;
		size_t i = 0u;
		for (auto& element : ilist)
		{
			new(&buffer[i]) Element(element);
			++i;
		}
	}

	void reserve(std::size_t new_cap)
	{
		if (new_cap <= mCapacity) return;
		resizeToKnownSizeNoChechs(new_cap);
	}

	std::size_t capacity() const noexcept
	{
		return mCapacity;
	}

	void shrink_to_fit()
	{
		if (mCapacity > size)
		{
			resizeToKnownSizeNoChechs(size);
		}
	}

	void clear() noexcept
	{
		if (size)
		{
			auto i = size;
			do
			{
				buffer[i].~Element();
				--i;
			} while (i);
			size = i;
		}
	}

	iterator insert(Element* pos, const Element& value)
	{
		if (pos >= &buffer[mCapacity])
		{
			std::size_t new_cap = pos - &buffer[0] + 1u;
			resizeToKnownSizeNoChechs(new_cap);
			*pos = value;
			for (; size != mCapacity - 1; ++size)
			{
				new (&buffer[size]) Element;
			}
			size = new_cap;
		}
		else if (pos >= &buffer[size])
		{
			*pos = value;
			for (Element* i = &buffer[size]; i < pos; ++i)
			{
				new (i) Element;
			}
			size = pos - &buffer[0] + 1u;
			
		}
		else
		{
			if (size == mCapacity)
			{
				resizeNoChecks();
			}
			for (Element* i = &buffer[size]; i > pos; --i)
			{
				new(i) Element(std::move(*(i - 1u)));
			}
			++size;
			*pos = value;
		}
	}

	iterator insert(Element* pos, Element&& value)
	{
		if (pos >= &buffer[mCapacity])
		{
			std::size_t new_cap = pos - &buffer[0] + 1u;
			resizeToKnownSizeNoChechs(new_cap);
			new(pos) Element(std::move(value));
			for (; size != mCapacity - 1; ++size)
			{
				new (&buffer[size]) Element;
			}
			size = new_cap;
		}
		else if (pos >= &buffer[size])
		{
			new(pos) Element(std::move(value));
			for (Element* i = &buffer[size]; i < pos; ++i)
			{
				new(i) Element;
			}
			size = pos - &buffer[0] + 1u;

		}
		else
		{
			if (size == mCapacity)
			{
				resizeNoChecks();
			}
			for (Element* i = &buffer[size]; i > pos; --i)
			{
				new(i) Element(std::move(*(i - 1u)));
				(i - 1u)->~Element();
			}
			++size;
			new(pos) Element(std::move(value));
		}
	}

	iterator insert(Element* pos, size_t count, const Element& value)
	{
		if (pos + count > &buffer[mCapacity])
		{
			std::size_t new_cap = (pos - &buffer[0]) + count;
			resizeToKnownSizeNoChechs(new_cap);
			if (pos >= &buffer[size])
			{
				for (size_t i = 0u; i < count; ++i)
				{
					new(&pos[i]) Element(value);
				}
				for (; size != new_cap - count; ++size)
				{
					new (&buffer[size]) Element;
				}
			}
			else
			{
				for (Element* i = &buffer[size] - 1u; i >= pos; --i)
				{
					new(&i[count]) Element(std::move(*i));
					i->~Element();
				}
				for (size_t i = 0u; i < count; ++i)
				{
					new(&pos[i]) Element(value);
				}
			}
			size = new_cap;
		}
		else if (pos >= &buffer[size])
		{
			for (size_t i = 0u; i < count; ++i)
			{
				new(&pos[i]) Element(value);
			}
			for (Element* i = &buffer[size]; i < pos; ++i)
			{
				new (i) Element;
			}
			size = pos - &buffer[0] + count;
		}
		else
		{
			if (size + count > mCapacity)
			{
				resizeNoChecks(size + count);
			}
			for (Element* i = &buffer[size] - 1u; i >= pos; --i)
			{
				new(&i[count]) Element(std::move(*i));
				i->~Element();
			}
			for (size_t i = 0u; i < count; ++i)
			{
				new(&pos[i]) Element(value);
			}
			size += count;
		}
	}

	template< typename InputIt >
	iterator insert(Element* pos, InputIt first, InputIt last)
	{
		size_t count = 0u;
		for (InputIt begin = first; begin != last; ++begin, ++count);

		if (pos + count > &buffer[mCapacity])
		{
			std::size_t new_cap = (pos - &buffer[0]) + count;
			resizeToKnownSizeNoChechs(new_cap);
			if (pos >= &buffer[size])
			{
				for (; first != last; ++first, ++pos)
				{
					new(pos) Element(*first);
				}
				for (; size != new_cap - count; ++size)
				{
					new (&buffer[size]) Element;
				}
			}
			else
			{
				for (Element* i = &buffer[size] - 1u; i >= pos; --i)
				{
					new(&i[count]) Element(std::move(*i));
					i->~Element();
				}
				for (; first != last; ++first, ++pos)
				{
					new(pos) Element(*first);
				}
			}
			size = new_cap;
		}
		else if (pos >= &buffer[size])
		{
			for (; first != last; ++first, ++pos)
			{
				new(pos) Element(*first);
			}
			for (Element* i = &buffer[size]; i < pos; ++i)
			{
				new (i) Element;
			}
			size = pos - &buffer[0] + count;
		}
		else
		{
			if (size + count > mCapacity)
			{
				resizeNoChecks(size + count);
			}
			for (Element* i = &buffer[size] - 1u; i >= pos; --i)
			{
				new(&i[count]) Element(std::move(*i));
				i->~Element();
			}
			for (; first != last; ++first, ++pos)
			{
				new(pos) Element(*first);
			}
			size += count;
		}
	}

	iterator insert(Element* pos, std::initializer_list<Element> ilist)
	{
		const auto count = ilist.size();
		if (pos + count > &buffer[mCapacity])
		{
			std::size_t new_cap = (pos - &buffer[0]) + count;
			resizeToKnownSizeNoChechs(new_cap);
			if (pos >= &buffer[size])
			{
				const auto end = ilist.end();
				for (auto start = ilist.begin(); start != end; ++start, ++pos)
				{
					new(pos) Element(*start);
				}
				for (; size != new_cap - count; ++size)
				{
					new (&buffer[size]) Element;
				}
			}
			else
			{
				for (Element* i = &buffer[size] - 1u; i >= pos; --i)
				{
					new(&i[count]) Element(std::move(*i));
					i->~Element();
				}
				const auto end = ilist.end();
				for (auto start = ilist.begin(); start != end; ++start, ++pos)
				{
					new(pos) Element(*start);
				}
			}
			size = new_cap;
		}
		else if (pos >= &buffer[size])
		{
			const auto end = ilist.end();
			for (auto start = ilist.begin(); start != end; ++start, ++pos)
			{
				new(pos) Element(*start);
			}
			for (Element* i = &buffer[size]; i < pos; ++i)
			{
				new (i) Element;
			}
			size = pos - &buffer[0] + count;
		}
		else
		{
			if (size + count > mCapacity)
			{
				resizeNoChecks(size + count);
			}
			for (Element* i = &buffer[size] - 1u; i >= pos; --i)
			{
				new(&i[count]) Element(std::move(*i));
				i->~Element();
			}
			const auto end = ilist.end();
			for (auto start = ilist.begin(); start != end; ++start, ++pos)
			{
				new(pos) Element(*start);
			}
			size += count;
		}
	}

	template< class... Args >
	iterator emplace(Element* pos, Args&&... args)
	{
		if (pos >= &buffer[mCapacity])
		{
			std::size_t new_cap = pos - &buffer[0] + 1u;
			resizeToKnownSizeNoChechs(new_cap);
			new(pos) Element(std::forward<Args>(args)...);
			for (; size != mCapacity - 1; ++size)
			{
				new (&buffer[size]) Element;
			}
			size = new_cap;
		}
		else if (pos >= &buffer[size])
		{
			new(pos) Element(std::forward<Args>(args)...);
			for (Element* i = &buffer[size]; i < pos; ++i)
			{
				new(i) Element;
			}
			size = pos - &buffer[0] + 1u;
		}
		else
		{
			if (size == mCapacity)
			{
				resizeNoChecks();
			}
			for (Element* i = &buffer[size]; i > pos; --i)
			{
				new(i) Element(std::move(*(i - 1u)));
				(i - 1u)->~Element();
			}
			++size;
			new(pos) Element(std::forward<Args>(args)...);
		}
	}

	iterator erase(Element* pos)
	{
		pos->~Element();
		--size;
		for (Element* i = pos; i < &buffer[size]; ++i)
		{
			new(i) Element(std::move(*(i + 1u)));
		}
		buffer[size].~Element();
	}

	iterator erase(Element* first, Element* last)
	{
		size_t numElements = 0u;
		for (; first != last; ++first, ++numElements)
		{
			first->~Element();
		}
		for (; last < &buffer[size]; ++last)
		{
			new(last - numElements) Element(std::move(*last));
		}
	}

	void push_back(Element& value)
	{
		if (size == mCapacity) resizeNoChecks();
		new(&buffer[size]) Element(value);
		++size;
	}

	void push_back(Element&& value)
	{
		if (size == mCapacity) resizeNoChecks();
		new(&buffer[size]) Element(std::move(value));
		++size;
	}

	template< typename... Args >
	reference emplace_back(Args&&... args)
	{
		if (size == mCapacity) resizeNoChecks();
		new(&buffer[size]) Element(std::forward(args)...);
		++size;
	}

	void pop_back()
	{
		buffer[size].~Element();
		--size;
	}

	void resize(size_t count)
	{
		if (count < size)
		{
			do
			{
				buffer[size].~Element();
				--size;
			} while (count < size);
		}
		else
		{
			if (count > mCapacity)
			{
				resizeToKnownSizeNoChechs(count);
			}
			for (; size < count; ++size)
			{
				new(&buffer[size]) Element;
			}
		}
	}

	void resize(size_t count, const Element& value)
	{
		if (count < size)
		{
			do
			{
				buffer[size].~Element();
				--size;
			} while (count < size);
		}
		else
		{
			if (count > mCapacity)
			{
				resizeToKnownSizeNoChechs(count);
			}
			for (; size < count; ++size)
			{
				new(&buffer[size]) Element(value);
			}
		}
	}
};

template<typename Element>
void swap(ResizingArray<Element>& lhs, ResizingArray<Element>& rhs)
{
	lhs.swap(rhs);
}