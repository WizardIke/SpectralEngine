#pragma once
#include <functional> //std::hash, std::equal_to
#include <cstddef> //std::size_t
#include <limits> //std::numeric_limits

/*
* Implements a set using hashing.
* Inserting an item does not invalidate any pointers, references or iterators into the FastIterationHashMap.
* Removing an item from a HashMap invalidates all pointers, references and iterators into the FastIterationHashMap.
* Iterating a FastIterationHashMap should take similar time as iterating a std::vector of the same length and is random access.
* SizeType should be an integral type
*/
template<class Key, class T, class Hasher = std::hash<Key>, class EqualTo = std::equal_to<Key>, class Allocator = std::allocator<Key>, class SizeType = std::size_t>
class FastIterationHashMap
{
public:
	using key_type = Key;
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

	using iterator = value_type * ;
	using const_iterator = const value_type*;
private:
	class Node
	{
	public:
		size_type distanceFromIdealPosition;
		size_type indexInData;
	};

	class ValueAllocatorWrapper : private std::allocator_traits<Allocator>::template rebind_alloc<T>
	{
		using Base1 = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
		using Traits = typename std::allocator_traits<Base1>;
	public:
		typename Traits::value_type* allocateValue(typename Traits::size_type n)
		{
			return Traits::allocate(static_cast<Base1&>(*this), n);
		}

		void deallocateValue(typename Traits::value_type* p, typename Traits::size_type n)
		{
			return Traits::deallocate(static_cast<Base1&>(*this), p, n);
		}
	};

	class KeyAllocatorWrapper : private std::allocator_traits<Allocator>::template rebind_alloc<Key>
	{
		using Base1 = typename std::allocator_traits<Allocator>::template rebind_alloc<Key>;
		using Traits = typename std::allocator_traits<Base1>;
	public:
		typename Traits::value_type* allocateKey(typename Traits::size_type n)
		{
			return Traits::allocate(static_cast<Base1&>(*this), n);
		}

		void deallocateKey(typename Traits::value_type* p, typename Traits::size_type n)
		{
			return Traits::deallocate(static_cast<Base1&>(*this), p, n);
		}
	};

	class lookUpAllocatorWrapper : private std::allocator_traits<Allocator>::template rebind_alloc<Node>
	{
		using Base = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
		using Traits = typename std::allocator_traits<Base>;
	public:
		typename Traits::value_type* allocateLookUp(typename Traits::size_type n)
		{
			return Traits::allocate(static_cast<Base&>(*this), n);
		}

		void deallocateLookUp(typename Traits::value_type* p, typename Traits::size_type n)
		{
			return Traits::deallocate(static_cast<Base&>(*this), p, n);
		}
	};

	struct HasherWrapper : private Hasher
	{
		template<class... Keys>
		auto hash(const Keys&... value) const -> decltype(std::declval<Hasher>()(value...))
		{
			return (*this)(value...);
		}
	};

	struct EqualToWrapper : private EqualTo
	{
		template<class... Keys>
		bool equal(const key_type& value1, const Keys&... value2) const
		{
			return (*this)(value1, value2...);
		}
	};

	struct Impl : private HasherWrapper, private EqualToWrapper, private ValueAllocatorWrapper, private KeyAllocatorWrapper, private lookUpAllocatorWrapper
	{
		using HasherWrapper::hash;
		using EqualToWrapper::equal;
		using ValueAllocatorWrapper::allocateValue;
		using ValueAllocatorWrapper::deallocateValue;
		using KeyAllocatorWrapper::allocateKey;
		using KeyAllocatorWrapper::deallocateKey;
		using lookUpAllocatorWrapper::allocateLookUp;
		using lookUpAllocatorWrapper::deallocateLookUp;

		FastIterationHashMap::pointer values;
		FastIterationHashMap::key_type* keys;
		FastIterationHashMap::Node* lookUp;
		FastIterationHashMap::size_type maxBucketCount;
		FastIterationHashMap::size_type size;
		FastIterationHashMap::size_type loadThreshold;
		float maxLoadFactor = 0.5f;
	};
	Impl impl;

	void rehashNoChecks(const size_type newMaxBucketCount, const size_type newLoadThreshold)
	{
		if(impl.size != 0u)
		{
			Node* newLookUp = impl.allocateLookUp(newMaxBucketCount);
			Node* const dataEnd = newLookUp + newMaxBucketCount;
			for(auto i = newLookUp; i != dataEnd; ++i)
			{
				i->distanceFromIdealPosition = 0u;
			}

			const Node* const oldDataEnd = impl.lookUp + impl.maxBucketCount;
			for(Node* n = impl.lookUp; n != oldDataEnd; ++n)
			{
				if(n->distanceFromIdealPosition != 0u)
				{
					auto hash = impl.hash(impl.keys[n->indexInData]);
					size_type bucketIndex = (size_type)(hash & (newMaxBucketCount - 1));
					swapInElement(n->indexInData, bucketIndex, newLookUp, newMaxBucketCount - 1u);
				}
			}
			impl.deallocateLookUp(impl.lookUp, impl.maxBucketCount);
			impl.lookUp = newLookUp;
			impl.maxBucketCount = newMaxBucketCount;

			pointer newValues = impl.allocateValue(newLoadThreshold);
			for(size_type i = 0u; i != newLoadThreshold; ++i)
			{
				newValues[i] = std::move(impl.values[i]);
				impl.values[i].~value_type();
			}
			impl.deallocateValue(impl.values, impl.loadThreshold);

			key_type* newKeys = impl.allocateKey(newLoadThreshold);
			for(size_type i = 0u; i != newLoadThreshold; ++i)
			{
				newKeys[i] = std::move(impl.keys[i]);
				impl.keys[i].~key_type();
			}
			impl.deallocateKey(impl.keys, impl.loadThreshold);

			impl.loadThreshold = newLoadThreshold;
		}
		else
		{
			//the hashmap is empty so we can deallocate its memory before allocating new memory
			if(impl.maxBucketCount != 0u)
			{
				impl.deallocateLookUp(impl.lookUp, impl.maxBucketCount);
				impl.deallocateValue(impl.values, impl.loadThreshold);
				impl.deallocateKey(impl.keys, impl.loadThreshold);
			}
			impl.maxBucketCount = newMaxBucketCount;
			impl.loadThreshold = newLoadThreshold;

			impl.lookUp = impl.allocateLookUp(newMaxBucketCount);
			Node* const dataEnd = impl.lookUp + newMaxBucketCount;
			for(auto i = impl.lookUp; i != dataEnd; ++i)
			{
				i->distanceFromIdealPosition = 0u;
			}

			impl.values = impl.allocateValue(newLoadThreshold);
			impl.keys = impl.allocateKey(newLoadThreshold);
		}
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

	static void swapInElement(size_type dataIndex, size_type bucketIndex, Node* lookUp, size_type mask)
	{
		size_type distanceFromIdealBucket = (size_type)1u;
		while(true)
		{
			if(lookUp[bucketIndex].distanceFromIdealPosition == (size_type)0u)
			{
				lookUp[bucketIndex].indexInData = dataIndex;
				lookUp[bucketIndex].distanceFromIdealPosition = distanceFromIdealBucket;
				break;
			}
			else if(lookUp[bucketIndex].distanceFromIdealPosition < distanceFromIdealBucket)
			{
				std::swap(dataIndex, lookUp[bucketIndex].indexInData);
				std::swap(distanceFromIdealBucket, lookUp[bucketIndex].distanceFromIdealPosition);
			}

			bucketIndex = (bucketIndex + (size_type)1u) & mask;
			++distanceFromIdealBucket;
		}
	}

	size_type findLookUpIndex(const key_type& key)
	{
		if(impl.maxBucketCount == 0u) return std::numeric_limits<size_type>::max();
		const size_type mask = impl.maxBucketCount - (size_type)1u;
		size_type bucketIndex = impl.hash(key) & mask;
		size_type distanceFromIdealBucket = 1u;
		while(impl.lookUp[bucketIndex].distanceFromIdealPosition >= distanceFromIdealBucket)
		{
			const size_type indexInData = impl.lookUp[bucketIndex].indexInData;
			const auto& bucketKey = impl.keys[indexInData];
			if(impl.lookUp[bucketIndex].distanceFromIdealPosition == distanceFromIdealBucket && impl.equal(bucketKey, key)) return bucketIndex;
			bucketIndex = (bucketIndex + (size_type)1u) & mask;
			++distanceFromIdealBucket;
		}
		return std::numeric_limits<size_type>::max();
	}

	/*
	returns capacity if not found
	*/
	size_type findIndexInData(const key_type& key)
	{
		if(impl.maxBucketCount == (size_type)0u) return (size_type)0u;
		const size_type mask = impl.maxBucketCount - (size_type)1u;
		size_type bucketIndex = impl.hash(key) & mask;
		size_type distanceFromIdealBucket = 1u;
		while(impl.lookUp[bucketIndex].distanceFromIdealPosition >= distanceFromIdealBucket)
		{
			const size_type indexInData = impl.lookUp[bucketIndex].indexInData;
			const auto& bucketKey = impl.keys[indexInData];
			if(impl.lookUp[bucketIndex].distanceFromIdealPosition == distanceFromIdealBucket && impl.equal(bucketKey, key)) return indexInData;
			bucketIndex = (bucketIndex + (size_type)1u) & mask;
			++distanceFromIdealBucket;
		}
		return impl.loadThreshold;
	}
public:
	FastIterationHashMap() noexcept
	{
		impl.values = nullptr;
		impl.keys = nullptr;
		impl.maxBucketCount = (size_type)0u;
		impl.size = (size_type)0u;
		impl.loadThreshold = (size_type)0u;
		impl.maxLoadFactor = 0.5f;
	}

	FastIterationHashMap(FastIterationHashMap&& other) noexcept : impl(std::move(other.impl)) 
	{
		other.impl.maxBucketCount = (size_type)0u;
	}

	FastIterationHashMap(const FastIterationHashMap& other)
	{
		impl.maxBucketCount = other.impl.maxBucketCount;
		impl.size = other.impl.size;
		impl.loadThreshold = other.impl.loadThreshold;
		impl.maxLoadFactor = other.impl.maxLoadFactor;
		if(impl.maxBucketCount != (size_type)0u)
		{
			impl.values = impl.allocateValue(impl.loadThreshold);
			impl.keys = impl.allocateKey(impl.loadThreshold);

			const auto currentSize = impl.size;
			for(size_type i = (size_type)0u; i != currentSize; ++i)
			{
				impl.values[i] = other.impl.values[i];
				impl.keys[i] = other.impl.keys[i];
			}
		}
		else
		{
			impl.values = nullptr;
			impl.keys = nullptr;
		}
	}

	~FastIterationHashMap()
	{
		if(impl.maxBucketCount != (size_type)0u)
		{
			const auto currentSize = impl.size;
			for(size_type i = (size_type)0u; i != currentSize; ++i)
			{
				impl.values[i].~value_type();
				impl.keys[i].~key_type();
			}
			impl.deallocateValue(impl.values, impl.loadThreshold);
			impl.deallocateKey(impl.keys, impl.loadThreshold);
			impl.deallocateLookUp(impl.lookUp, impl.maxBucketCount);
		}
	}

	void operator=(FastIterationHashMap&& other)
	{
		this->~FastIterationHashMap();
		new(this) FastIterationHashMap(std::move(other));
	}

	void operator=(const FastIterationHashMap& other)
	{
		this->~FastIterationHashMap();
		new(this) FastIterationHashMap(other);
	}

	iterator begin()
	{
		return impl.values;
	}

	const_iterator begin() const
	{
		return impl.values;
	}

	const_iterator cbegin() const
	{
		return impl.values;
	}

	iterator end()
	{
		return impl.values + impl.size;
	}

	const_iterator end() const
	{
		return impl.values + impl.size;
	}

	const_iterator cend() const
	{
		return impl.values + impl.size;
	}

	bool empty() const noexcept
	{
		return impl.size == (size_type)0u;
	}

	size_type size() const noexcept
	{
		return impl.size;
	}

	size_type capacity() const noexcept
	{
		return impl.loadThreshold;
	}

	size_type max_size() const noexcept
	{
		return (size_type)(impl.maxLoadFactor * std::numeric_limits<size_type>::max());
	}

	void insert(key_type&& key, value_type&& value)
	{
		if(impl.size == impl.loadThreshold)
		{
			auto nextMaxBucketCountAndLoadThreshold = nextCapacity(impl.maxBucketCount, impl.loadThreshold, impl.maxLoadFactor);
			rehashNoChecks(nextMaxBucketCountAndLoadThreshold.maxBucketCount, nextMaxBucketCountAndLoadThreshold.loadThreshold);
		}
		const size_type dataIndex = impl.size;
		const size_type bucketIndex = (size_type)(impl.hash(key) & (impl.maxBucketCount - 1u));
		++impl.size;
		impl.keys[dataIndex] = std::move(key);
		impl.values[dataIndex] = std::move(value);
		swapInElement(dataIndex, bucketIndex, impl.lookUp, impl.maxBucketCount - 1u);
	}

	void insert(const key_type& key, const value_type& value)
	{
		if(impl.size == impl.loadThreshold)
		{
			auto nextMaxBucketCountAndLoadThreshold = nextCapacity(impl.maxBucketCount, impl.loadThreshold, impl.maxLoadFactor);
			rehashNoChecks(nextMaxBucketCountAndLoadThreshold.maxBucketCount, nextMaxBucketCountAndLoadThreshold.loadThreshold);
		}
		const size_type dataIndex = impl.size;
		const size_type bucketIndex = (size_type)(impl.hash(key) & (impl.maxBucketCount - 1u));
		++impl.size;
		impl.keys[dataIndex] = key;
		impl.values[dataIndex] = value;
		swapInElement(dataIndex, bucketIndex, impl.lookUp, impl.maxBucketCount - 1u);
	}

	void erase(const key_type& key)
	{
		/**
		* Shift items back into the new empty space until we reach an empty space or an item in its ideal position.
		*/
		size_type previousBucket = findLookUpIndex(key);
		if(previousBucket == std::numeric_limits<size_type>::max())
		{
			return;
		}
		const auto mask = impl.maxBucketCount - (size_type)1u;
		size_type bucket = (previousBucket + (size_type)1u) & mask;

		const auto indexInData = impl.lookUp[previousBucket].indexInData;
		--impl.size;
		const auto size = impl.size;
		if(indexInData != size)
		{
			impl.values[indexInData] = impl.values[std::move(size)];
			impl.keys[indexInData] = impl.keys[std::move(size)];
		}
		impl.values[size].~value_type();
		impl.keys[size].~key_type();

		while(impl.lookUp[bucket].distanceFromIdealPosition > 1u)
		{
			const size_type newDistance = impl.lookUp[bucket].distanceFromIdealPosition - 1u;
			impl.lookUp[previousBucket].distanceFromIdealPosition = newDistance;
			impl.lookUp[previousBucket].indexInData = impl.lookUp[bucket].indexInData;

			previousBucket = bucket;
			bucket = (previousBucket + 1u) & mask;
		}
		impl.lookUp[previousBucket].distanceFromIdealPosition = 0u;
	}

	iterator find(const key_type& key)
	{
		const size_type indexInData = findIndexInData(key);
		return impl.values + indexInData;
	}
};