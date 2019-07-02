#pragma once
#include <memory>
#include <functional> //std::hash, std::equal_to
#include <limits>
#include <type_traits>
#include <cstddef>
#include <algorithm>

/*
* Implements a set using hashing.
* Inserting or removing an item from a HashMap invalidates all pointers, references and iterators into the HashMap.
*/
template<
	class T, 
	class Hasher = std::hash<T>, 
	class EqualTo = std::equal_to<T>, 
	class Allocator = std::allocator<T>,
	class SizeType = std::size_t
>
class HashSet
{
public:
	using key_type = T;
	using value_type = T;
	using size_type = SizeType;
	using difference_type = std::ptrdiff_t;
	using hasher = Hasher;
	using key_equal = EqualTo;
	using allocator_type = Allocator;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
private:
	class Node
	{
		size_type mDistanceFromIdealPosition; //starts from 1 when the element is in its ideal position. 0 is reserved for an empty Node.
		union //unions don't call constructors and destructors automatically
		{
			value_type mData;
		};
	public:
		void operator=(const Node& other)
		{
			mDistanceFromIdealPosition = other.mDistanceFromIdealPosition;
			if (mDistanceFromIdealPosition != 0u)
			{
				mData = other.mData;
			}
		}

		value_type& data()
		{
			return mData;
		}

		const value_type& data() const
		{
			return mData;
		}

		size_type& distanceFromIdealPosition()
		{
			return mDistanceFromIdealPosition;
		}
	};
	
	template<class T2>
	class Iterator
	{
		template<class T, class Hasher, class EqualTo, class Allocator, class SizeType>
		friend class HashSet;
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
public:
		using iterator = Iterator<value_type>;
		using const_iterator = Iterator<const value_type>;
private:
	
	struct HasherWrapper : private Hasher
	{
		template<class... Key>
		auto hash(const Key&... value) const -> decltype(std::declval<Hasher>()(value...))
		{
			return (*this)(value...);
		}
	};
	
	struct EqualToWrapper : private EqualTo
	{
		template<class... Keys>
		bool equal(const T& value1, const Keys&... value2) const
		{
			return (*this)(value1, value2...);
		}
	};
	
	class AllocatorWrapper : private std::allocator_traits<Allocator>::template rebind_alloc<Node>
	{
		using Base = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
		using Traits = std::allocator_traits<typename Base>;
	public:
		typename Traits::value_type* allocate(typename Traits::size_type n)
		{
			return Traits::allocate(*static_cast<Base*>(this), n);
		}

		void deallocate(typename Traits::value_type* p, typename Traits::size_type n)
		{
			return Traits::deallocate(*static_cast<Base*>(this), p, n);
		}
	};
	
	struct Impl : public HasherWrapper, public EqualToWrapper, public AllocatorWrapper
	{
		using HasherWrapper::hash;
		using EqualToWrapper::equal;
		using AllocatorWrapper::allocate;
		using AllocatorWrapper::deallocate;

		HashSet::Node* data = nullptr;
		HashSet::size_type maxBucketCount = 0u;
		HashSet::size_type mSize = 0u;
		HashSet::size_type loadThreshold = 0u;
		float maxLoadFactor = 0.5f;
	};
	Impl impl;
	
	void rehashNoChecks(const size_type newMaxBucketCount, const size_type newLoadThreshold)
	{
		const size_type oldMaxBucketCount = impl.maxBucketCount;
		Node* oldData = impl.data;

		impl.maxBucketCount = newMaxBucketCount;
		impl.loadThreshold = newLoadThreshold;

		if(impl.mSize != 0u)
		{
			impl.data = impl.allocate(newMaxBucketCount + 1u);
			impl.data[newMaxBucketCount].distanceFromIdealPosition() = 1u; //must not be zero so that iteration stops properly.

			Node* const dataEnd = impl.data + impl.maxBucketCount;
			for(auto i = impl.data; i != dataEnd; ++i)
			{
				i->distanceFromIdealPosition() = 0u;
			}

			const Node* const oldDataEnd = oldData + oldMaxBucketCount;
			for(Node* n = oldData; n != oldDataEnd; ++n)
			{
				if(n->distanceFromIdealPosition() != 0u)
				{
					auto hash = impl.hash(n->data());
					Node* bucket = impl.data + (hash & (impl.maxBucketCount - 1));
					swapInElement(std::move(n->data()), bucket, dataEnd, impl.data);
					n->data().~value_type();
				}
			}

			impl.deallocate(oldData, oldMaxBucketCount + 1u);
		}
		else
		{
			//If the hashmap is empty we can deallocate its memory before allocating new memory
			if(oldMaxBucketCount != 0u)
			{
				impl.deallocate(oldData, oldMaxBucketCount + 1u);
			}

			impl.data = impl.allocate(newMaxBucketCount + 1u);
			impl.data[newMaxBucketCount].distanceFromIdealPosition() = 1u; //must not be zero so that iteration stops properly.

			Node* const dataEnd = impl.data + impl.maxBucketCount;
			for(auto i = impl.data; i != dataEnd; ++i)
			{
				i->distanceFromIdealPosition() = 0u;
			}
		}
	}

	static void swapInElement(value_type&& value, Node* bucket, Node* const dataEnd, Node* const dataStart)
	{
		size_type distanceFromIdealBucket = 1u;
		while(true)
		{
			if(bucket->distanceFromIdealPosition() == 0)
			{
				bucket->data() = std::move(value);
				bucket->distanceFromIdealPosition() = distanceFromIdealBucket;
				break;
			}
			else if(bucket->distanceFromIdealPosition() < distanceFromIdealBucket)
			{
				using std::swap;
				swap(value, bucket->data());
				std::swap(bucket->distanceFromIdealPosition(), distanceFromIdealBucket);
			}

			++bucket;
			if(bucket == dataEnd) bucket = dataStart;
			++distanceFromIdealBucket;
		}
	}

	static iterator swapInElementAndGet(value_type&& value, Node* bucket, Node* const dataEnd, Node* const dataStart)
	{
		size_type distanceFromIdealBucket = 1u;
		while(true)
		{
			if(bucket->distanceFromIdealPosition() == 0)
			{
				bucket->data() = std::move(value);
				bucket->distanceFromIdealPosition() = distanceFromIdealBucket;
				return iterator(bucket);
			}
			else if(bucket->distanceFromIdealPosition() < distanceFromIdealBucket)
			{
				using std::swap;
				swap(value, bucket->data());
				std::swap(bucket->distanceFromIdealPosition(), distanceFromIdealBucket);
			}

			++bucket;
			if(bucket == dataEnd) bucket = dataStart;
			++distanceFromIdealBucket;
		}
	}

	template<class... Key>
	Node* findImpl(const Key&... value) const
	{
		if(impl.maxBucketCount == 0u) return nullptr;
		auto hash = impl.hash(value...);
		Node* bucket = impl.data + (hash & (impl.maxBucketCount - 1));
		Node* const endData = impl.data + impl.maxBucketCount;
		size_type distanceFromIdealBucket = 1u;
		while(bucket->distanceFromIdealPosition() >= distanceFromIdealBucket)
		{
			if(bucket->distanceFromIdealPosition() == distanceFromIdealBucket && impl.equal(bucket->data(), value...)) return bucket;
			++bucket;
			if(bucket == endData) bucket = impl.data;
			++distanceFromIdealBucket;
		}
		return endData;
	}

	static size_type roundUpToPowerOf2(size_type v)
	{
		--v;
		constexpr auto numberOfBitsInSizeType = (size_type)(sizeof(size_type) * CHAR_BIT);
		for(size_type i = (size_type)1u; i != numberOfBitsInSizeType; i *= (size_type)2u)
		{
			v |= v >> i;
		}
		++v;
		return v;
	}

	class MaxBucketCountAndLoadThreshold
	{
	public:
		size_type maxBucketCount;
		size_type loadThreshold;
	};

	static MaxBucketCountAndLoadThreshold nextCapacity(size_type currentMaxBucketCount, size_type currentLoadThreshold, float maxLoadFactor)
	{
		if(currentMaxBucketCount == 0u)
		{
			size_type newMaxBucketCount = std::max(roundUpToPowerOf2((size_type)((1.0f / maxLoadFactor) + (size_type)1u)), (size_type)8u);
			return {newMaxBucketCount, (size_type)(newMaxBucketCount * (double)maxLoadFactor)};
		}
		else
		{
			return {currentMaxBucketCount * 2u, currentLoadThreshold * 2u};
		}
	}
public:
	HashSet() noexcept {}

	HashSet(HashSet&& other) noexcept : impl(std::move(other.impl))
	{
		other.impl.maxBucketCount = 0u;
	}

	HashSet(const HashSet& other)
	{
		if (other.impl.mSize != 0u)
		{
			impl.data = impl.allocate(other.impl.maxBucketCount + 1u);
			impl.maxBucketCount = other.impl.maxBucketCount;
			impl.mSize = other.impl.mSize;
			impl.loadThreshold = other.impl.loadThreshold;
			impl.maxLoadFactor = other.impl.maxLoadFactor;
			for (size_type i = 0u; i != impl.maxBucketCount; ++i)
			{
				impl.data[i] = other.impl.data[i];
			}
		}
	}

	~HashSet()
	{
		if (impl.maxBucketCount != 0u)
		{
			if(impl.mSize != 0u)
			{
				const auto dataEnd = impl.data + impl.maxBucketCount;
				for(Node* n = impl.data; n != dataEnd; ++n)
				{
					if(n->distanceFromIdealPosition() != 0u)
					{
						n->data().~value_type();
					}
				}
			}
			impl.deallocate(impl.data, impl.maxBucketCount + 1u);
		}
	}

	void operator=(HashSet&& other)
	{
		this->~HashSet();
		new(this) HashSet(std::move(other));
	}

	void operator=(const HashSet& other)
	{
		this->~HashSet();
		new(this) HashSet(other);
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
			return const_iterator(current);
		}
		return const_iterator(nullptr);
	}
	
	const_iterator cbegin() const
	{
		return begin();
	}

	iterator end()
	{
		return iterator(impl.data + impl.maxBucketCount);
	}

	const_iterator end() const
	{
		return const_iterator(impl.data + impl.maxBucketCount);
	}
	
	const_iterator cend() const
	{
		return const_iterator(impl.data + impl.maxBucketCount);
	}

	bool empty() const noexcept
	{
		return impl.mSize == 0u;
	}

	size_type size() const noexcept
	{
		return impl.mSize;
	}

	size_type loadThreshold() const noexcept
	{
		return impl.loadThreshold;
	}

	size_type max_size() const noexcept
	{
		return (size_type)(impl.maxLoadFactor * std::numeric_limits<size_type>::max());
	}

	void insert(value_type value)
	{
		if(impl.mSize == impl.loadThreshold)
		{
			auto nextMaxBucketCountAndLoadThreshold = nextCapacity(impl.maxBucketCount, impl.loadThreshold, impl.maxLoadFactor);
			rehashNoChecks(nextMaxBucketCountAndLoadThreshold.maxBucketCount, nextMaxBucketCountAndLoadThreshold.loadThreshold);
		}
			
		++impl.mSize;
		auto hash = impl.hash(value);
		Node* bucket = impl.data + (hash & (impl.maxBucketCount - 1));
		Node* const endData = impl.data + impl.maxBucketCount;
		swapInElement(std::move(value), bucket, endData, impl.data);
	}

	iterator insertAndGet(value_type value)
	{
		if(impl.mSize == impl.loadThreshold)
		{
			auto nextMaxBucketCountAndLoadThreshold = nextCapacity(impl.maxBucketCount, impl.loadThreshold, impl.maxLoadFactor);
			rehashNoChecks(nextMaxBucketCountAndLoadThreshold.maxBucketCount, nextMaxBucketCountAndLoadThreshold.loadThreshold);
		}

		++impl.mSize;
		auto hash = impl.hash(value);
		Node* bucket = impl.data + (hash & (impl.maxBucketCount - 1));
		Node* const endData = impl.data + impl.maxBucketCount;
		return swapInElementAndGet(std::move(value), bucket, endData, impl.data);
	}

	template<class... Key>
	iterator find(const Key&... value)
	{
		return iterator(findImpl(value...));
	}

	template<class... Key>
	const_iterator find(const Key&... value) const
	{
		return const_iterator(findImpl(value...));
	}

	template<class... Key>
	bool contains(const Key&... values) const
	{
		return findImpl(values...) != impl.data + impl.maxBucketCount;
	}

	void clear()
	{
		if(impl.mSize != 0u)
		{
			const auto dataEnd = impl.data + impl.maxBucketCount;
			for(auto current = impl.data; current != dataEnd; ++current)
			{
				if(current->distanceFromIdealPosition() != 0u)
				{
					current->data().~value_type();
					current->distanceFromIdealPosition() = 0u;
				}
			}
			impl.mSize = 0u;
		}
	}

	void erase(const_iterator pos)
	{
		/**
		* Shift items back into the new empty space until we reach an empty space or an item in its ideal position.
		*/
		size_type previousBucket = (size_type)(pos.ptr - impl.data);
		size_type bucket = (previousBucket + 1u) & (impl.maxBucketCount - 1u);

		impl.data[previousBucket].data().~value_type();
		--impl.mSize;

		while (impl.data[bucket].distanceFromIdealPosition() > 1u)
		{
			const size_type newDistance = impl.data[bucket].distanceFromIdealPosition() - 1u;
			impl.data[previousBucket].distanceFromIdealPosition() = newDistance;
			new(&impl.data[previousBucket].data()) value_type(std::move(impl.data[bucket].data()));
			impl.data[bucket].data().~value_type();

			previousBucket = bucket;
			bucket = (previousBucket + 1u) & (impl.maxBucketCount - 1u);
		}
		impl.data[previousBucket].distanceFromIdealPosition() = 0u;
	}
	
	void erase(iterator pos)
	{
		erase((const_iterator)pos);
	}

	template<class... Key>
	void erase(const Key&... values)
	{
		const_iterator it = find(values...);
		if(it != end())
		{
			erase(it);
		}
	}

	template<class F>
	void consume(F&& f)
	{
		Node* const endData = impl.data + impl.maxBucketCount;
		Node* current = impl.data;
		while(current != endData)
		{
			while(current->distanceFromIdealPosition() == 0u)
			{
				++current;
			}
			if (current == endData)
			{
				impl.mSize = (size_type)0u;
				return;
			}
			f(std::move(current->data()));
			current->data().~value_type();
			current->distanceFromIdealPosition() = 0u;
		}
	}

	void shrink_to_size(size_type size)
	{
		assert(size >= impl.mSize);
		if(size >= impl.loadThreshold) return; //already small enough
		const size_type newMaxBucketCount = roundUpToPowerOf2((size_type)((double)size / impl.maxLoadFactor) + (size_type)1u);
		rehashNoChecks(newMaxBucketCount, (size_type)(newMaxBucketCount * (double)impl.maxLoadFactor));
	}
};