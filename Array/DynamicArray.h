#pragma once
#include "ArrayBase.h"
#include "Buffer.h"
#include "ArraySize.h"
#include <initializer_list>
#include "ArrayPointer.h"

template<class Element>
class DynamicArray : public ArrayBase<Element>
{
public:
	class DoNotInitialize {};

	DynamicArray(const ArraySize size, DoNotInitialize) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[size.size]), size.size) {}

	DynamicArray(const ArraySize size) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[size.size]), size.size) 
	{
		std::size_t i;
		try
		{
			for (i = 0u; i < size.size; ++i)
			{
				new(&buffer[i]) Element;
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
	DynamicArray(const ArraySize size, Functor& initializer, typename std::enable_if<true, Return>::type* = nullptr) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[size.size]), size.size) 
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
	
	template<typename Functor, typename = typename std::result_of_t<Functor(std::size_t, Element&)> >
	DynamicArray(const ArraySize size, Functor& initializer) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[size.size]), size.size)
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

	DynamicArray(const std::initializer_list<Element> initializer) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[initializer.size()]),
			initializer.size())
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

	DynamicArray(std::initializer_list<Element>&& initializer) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[initializer.size()]),
			initializer.size())
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

	DynamicArray(const ArraySize size, const std::initializer_list<Element> initializer) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[initializer.size() > size.size ? initializer.size() :
			size.size]), size.size)
	{
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

	DynamicArray(const ArraySize size, std::initializer_list<Element>&& initializer) :
		ArrayBase(reinterpret_cast<Element*>(new Memory<sizeof(Element), alignof(Element)>[initializer.size() > size.size ? initializer.size() :
			size.size]), size.size)
	{
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

	~DynamicArray()
	{
		if (mCapacity != 0)
		{
			std::size_t i = mCapacity;
			do
			{
				buffer[i].~Element();
				--i;
			} while (i != 0);
		}
		delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
	}

	operator ArrayPointer<Element>() { return *(static_cast<ArrayPointer<Element>*>(this)); }

	constexpr bool empty() const noexcept
	{
		return false;
	}

	void swap(DynamicArray& other) // noexcept(noexcept(std::swap(declval<ElementType&>(), declval<ElementType&>())))
	{
		union
		{
			std::size_t size;
			Element* buffer;
		};
		size = this->mCapacity;
		this->mCapacity = other.mCapacity;
		other.mCapacity = size;

		buffer = this->buffer;
		this->buffer = other.buffer;
		other.buffer = buffer;
	}

	const DynamicArray& operator=(DynamicArray&& other)
	{
		delete[] reinterpret_cast<Memory<sizeof(Element), alignof(Element)>*>(buffer);
		this->mCapacity = other.mCapacity;
		this->buffer = other.buffer;
		other.buffer = nullptr;
		return *this;
	}

	std::size_t max_size() const noexcept
	{
		return mCapacity;
	}
};

template<class Element>
void swap(DynamicArray<Element>& lhs, DynamicArray<Element>& rhs)
{
	lhs.swap(rhs);
}