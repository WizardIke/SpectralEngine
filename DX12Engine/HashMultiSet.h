#pragma once
#include <memory>
#include <limits>
#include <type_traits>
#include <cstddef>

/*
* Implements a set using hashing.
* Inserting or removing an item from a HashMap invalidates all pointers, references and iterators into the HashMap.
*/
template<class T, class Hasher = std::hash<T>, class EqualTo = std::equal_to<T>, class Allocator = std::allocator<T>>
class HashMultiSet
{
public:
	using key_type = T;
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using hasher = Hasher;
	using key_equal = EqualTo;
	using allocator_type = Allocator;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
private:
	template<size_t size, size_t alignment>
	class Memory
	{
		alignas(alignment) char data[size];
	};
	
	class Node
	{
		size_type mDistanceFromIdealPosition;
		Memory<sizeof(value_type), alignof(value_type)> mData;
	public:
		void operator=(const Node& other)
		{
			mDistanceFromIdealPosition = other.mDistanceFromIdealPosition;
			if (mDistanceFromIdealPosition != 0u)
			{
				data() = other.data();
			}
		}

		value_type& data()
		{
			return *reinterpret_cast<value_type*>(&mData);
		}

		const value_type& data() const
		{
			return *reinterpret_cast<const value_type*>(&mData);
		}

		size_type& distanceFromIdealPosition()
		{
			return mDistanceFromIdealPosition;
		}
	};
	
	template<class T2>
	class Iterator
	{
		template<class T, class Hasher, class EqualTo, class Allocator>
		friend class HashMultiSet;
		template<typename U>
		friend class Iterator;
		Node* ptr;
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = std::remove_const_t<T2>;
		using pointer = T2*;
		using reference = T2&;
		using iterator_category = std::bidirectional_iterator_tag;

		Iterator(Node* ptr1) : ptr(ptr1) {}

		template<class OtherType, class Tag = typename std::enable_if<std::is_same<OtherType, T2>::value || std::is_same<OtherType, typename std::remove_const<T2>::type>::value>::type>
		Iterator(const Iterator<OtherType>& other) : ptr(other.ptr) {}

		Iterator operator++()
		{
			do
			{
				++ptr;

			} while (ptr->distanceFromIdealPosition() == 0u);
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator ret(ptr);
			do
			{
				++ptr;
			} while (ptr->distanceFromIdealPosition() == 0u);
			return ret;
		}

		Iterator operator--()
		{
			do
			{
				--ptr;
			} while (ptr->distanceFromIdealPosition() == 0u);
			return *this;
		}

		Iterator operator--(int)
		{
			Iterator ret(ptr);
			do
			{
				--ptr;
			} while (ptr->distanceFromIdealPosition() == 0u);
			return ret;
		}

		bool operator==(const Iterator& other) const noexcept
		{
			return ptr == other.ptr;
		}

		bool operator!=(const Iterator& other) const noexcept
		{
			return ptr != other.ptr;
		}

		reference operator*() const
		{
			return ptr->data();
		}

		pointer operator->()
		{
			return &ptr->data();
		}
	};
	
	struct HasherWrapper : private Hasher
	{
		template<class Key>
		auto hash(const Key& value) -> decltype(std::declval<Hasher>()(value))
		{
			return (*this)(value);
		}
	};
	
	struct EqualToWrapper : private EqualTo
	{
		template<class Key>
		bool equal(const T& value1, const Key& value2)
		{
			return (*this)(value1, value2);
		}
	};
	
	struct AllocatorWrapper : public std::allocator_traits<Allocator>::template rebind_alloc<Node>
	{
	};
	
	struct Impl : public HasherWrapper, public EqualToWrapper, public AllocatorWrapper
	{
		Node* data = nullptr;
		size_type capacity = 0u;
		size_type mSize = 0u;
		size_type loadThreshold = 0u;
		float loadFactor = 0.5f;
	};
	Impl impl;
	
	void rehashNoChecks(size_type newCapacity)
	{
		if (impl.capacity == 0u)
		{
			newCapacity = 8u;
		}

		size_type oldCapacity = impl.capacity;
		Node* oldData = impl.data;

		impl.capacity = newCapacity;
		impl.data = impl.allocate(newCapacity + 1u);
		impl.data[newCapacity].distanceFromIdealPosition() = 1u;
		impl.mSize = 0u;
		impl.loadThreshold = (size_type)(newCapacity * (double)impl.loadFactor);

		const Node* const dataEnd = impl.data + impl.capacity;
		for (auto i = impl.data; i != dataEnd; ++i)
		{
			i->distanceFromIdealPosition() = 0u;
		}

		if (oldCapacity != 0u)
		{
			const Node* const oldDataEnd = oldData + oldCapacity;
			for (Node* n = oldData; n != oldDataEnd; ++n)
			{
				if (n->distanceFromIdealPosition() != 0u)
				{
					insert(std::move(n->data()));
					n->data().~value_type();
				}
				n->~Node();
			}

			impl.deallocate(oldData, oldCapacity + 1u);
		}
	}
public:
	using iterator = Iterator<value_type>;
	using const_iterator = Iterator<const value_type>;

	HashMultiSet() noexcept {}

	HashMultiSet(HashMultiSet&& other) noexcept : impl(std::move(other.impl)) 
	{
		other.impl.capacity = 0u;
	}

	HashMultiSet(const HashMultiSet& other)
	{
		if (other.impl.mSize != 0)
		{
			impl.data = impl.allocate(other.impl.capacity);
			impl.capacity = other.impl.capacity;
			impl.mSize = other.impl.mSize;
			impl.loadThreshold = other.impl.loadThreshold;
			impl.loadFactor = other.impl.loadFactor;
			for (size_type i = 0u; i != impl.capacity; ++i)
			{
				impl.data[i] = other.impl.data[i];
			}
		}
	}

	~HashMultiSet()
	{
		if (impl.capacity != 0u)
		{
			const auto dataEnd = impl.data + impl.capacity;
			for (Node* n = impl.data; n != dataEnd; ++n)
			{
				if (n->distanceFromIdealPosition() != 0u)
				{
					n->data().~value_type();
				}
				n->~Node();
			}
			impl.deallocate(impl.data, impl.capacity + 1u);
		}
	}

	void operator=(HashMultiSet&& other)
	{
		this->~HashMultiSet();
		new(this) HashMultiSet(std::move(other));
	}

	void operator=(const HashMultiSet& other)
	{
		this->~HashMultiSet();
		new(this) HashMultiSet(other);
	}

	iterator begin()
	{
		if (impl.data != nullptr)
		{
			Node* current = impl.data;
			while (current->distanceFromIdealPosition() == 0u)
			{
				++current;
			}
			return iterator(current);
		}
		return iterator(nullptr);
	}

	const_iterator begin() const
	{
		if (impl.data != nullptr)
		{
			Node* current = impl.data;
			while (current->distanceFromIdealPosition() == 0u)
			{
				++current;
			}
			return iterator(current);
		}
		return iterator(nullptr);
	}
	
	const_iterator cbegin() const
	{
		return begin();
	}

	iterator end()
	{
		return iterator(impl.data + impl.capacity);
	}

	const_iterator end() const
	{
		return const_iterator(impl.data + impl.capacity);
	}
	
	const_iterator cend() const
	{
		return const_iterator(impl.data + impl.capacity);
	}

	bool empty() const noexcept
	{
		return impl.mSize == 0u;
	}

	size_type size() const noexcept
	{
		return impl.mSize;
	}

	size_type max_size() const noexcept
	{
		return (size_type)(impl.loadFactor * std::numeric_limits<size_type>::max());
	}

	void insert(value_type value)
	{
		if (impl.mSize == impl.loadThreshold) rehashNoChecks(impl.capacity * 2u);
		++impl.mSize;
		auto hash = impl.hash(value);
		Node* bucket = impl.data + (hash & (impl.capacity - 1));
		size_type distanceFromIdealBucket = 1u;
		Node* const endData = impl.data + impl.capacity;
		while (true)
		{
			if (bucket->distanceFromIdealPosition() == 0)
			{
				bucket->data() = std::move(value);
				bucket->distanceFromIdealPosition() = distanceFromIdealBucket;
				break;
			}
			else if (bucket->distanceFromIdealPosition() < distanceFromIdealBucket)
			{
				std::swap(value, bucket->data());
				std::swap(bucket->distanceFromIdealPosition(), distanceFromIdealBucket);
			}

			++bucket;
			if (bucket == endData) bucket = impl.data;
			++distanceFromIdealBucket;
		}
	}

	template<class Key>
	iterator find(const Key& value)
	{
		if (impl.capacity == 0u) return iterator(nullptr);
		auto hash = impl.hash(value);
		Node* bucket = impl.data + (hash & (impl.capacity - 1));
		size_type distanceFromIdealBucket = 1u;
		Node* const endData = impl.data + impl.capacity;
		while (bucket->distanceFromIdealPosition() >= distanceFromIdealBucket)
		{
			if(bucket->distanceFromIdealPosition() == distanceFromIdealBucket && impl.equal(bucket->data(), value)) return iterator(bucket);
			++bucket;
			if (bucket == endData) bucket = impl.data;
			++distanceFromIdealBucket;
		}
		
		return iterator(endData);
	}

	void clear()
	{
		const auto dataEnd = impl.data + impl.capacity;
		for (auto current = impl.data; current != dataEnd; ++current)
		{
			if (current->distanceFromIdealPosition() != 0u)
			{
				current->~Node();
				current->distanceFromIdealPosition() = 0u;
			}
		}
	}

	void erase(const_iterator pos)
	{
		/**
		* Shift items back into the new empty space until we reach an empty space or an item in its ideal position.
		*/
		std::size_t previousBucket = pos.ptr - impl.data;
		std::size_t bucket = (previousBucket + 1u) & (impl.capacity - 1u);

		impl.data[previousBucket].data().~value_type();
		--impl.mSize;

		while (impl.data[bucket].distanceFromIdealPosition() > 1u)
		{
			const size_type newDistance = impl.data[bucket].distanceFromIdealPosition() - 1u;
			impl.data[previousBucket].distanceFromIdealPosition() = newDistance;
			new(&impl.data[previousBucket].data()) value_type(std::move(impl.data[bucket].data()));
			impl.data[bucket].data().~value_type();

			previousBucket = bucket;
			bucket = (previousBucket + 1u) & (impl.capacity - 1u);
		}
		impl.data[previousBucket].distanceFromIdealPosition() = 0u;
	}
	
	void erase(iterator pos)
	{
		erase((const_iterator)pos);
	}

	template<class Key>
	void erase(const Key& value)
	{
		erase(find(value));
	}
};