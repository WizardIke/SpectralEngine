#pragma once
#include <functional> //std::hash, std::equal_to
#include <cstddef> //std::size_t
#include <limits> //std::numeric_limits
#include <type_traits> //std::is_trivially_destructible_v

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

	using iterator = value_type*;
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

		using value_type = FastIterationHashMap::value_type;
		using key_type = FastIterationHashMap::key_type;
		using size_type = FastIterationHashMap::size_type;
		using Node = FastIterationHashMap::Node;

		value_type* values;
		key_type* keys;
		Node* lookUp;
		size_type maxBucketCount;
		size_type size;
		size_type loadThreshold;
		float maxLoadFactor;

	private:
		void copyHelper(const Impl& other)
		{
			maxBucketCount = other.maxBucketCount;
			size = other.size;
			loadThreshold = other.loadThreshold;
			maxLoadFactor = other.maxLoadFactor;
			if (maxBucketCount != (size_type)0u)
			{
				values = allocateValue(loadThreshold);
				keys = allocateKey(loadThreshold);
				const auto currentSize = size;
				for (size_type i = (size_type)0u; i != currentSize; ++i)
				{
					new(&values[i]) value_type{ other.values[i] };
					new(&keys[i]) key_type{ other.keys[i] };
				}

				lookUp = allocateLookUp(maxBucketCount);
				const auto maxBucketCountTemp = maxBucketCount;
				for (size_type i = (size_type)0u; i != maxBucketCountTemp; ++i)
				{
					new(lookUp + i) Node{ other.lookUp[i] };
				}
			}
			else
			{
				values = nullptr;
				keys = nullptr;
				lookUp = nullptr;
			}
		}

		void destruct()
		{
			if (maxBucketCount != (size_type)0u)
			{
				const auto currentSize = size;
				for (size_type i = (size_type)0u; i != currentSize; ++i)
				{
					values[i].~value_type();
					keys[i].~key_type();
				}
				deallocateValue(values, loadThreshold);
				deallocateKey(keys, loadThreshold);
				static_assert(std::is_trivially_destructible_v<Node>);
				deallocateLookUp(lookUp, maxBucketCount);
			}
		}
	public:

		Impl() noexcept :
			values(nullptr),
			keys(nullptr),
			lookUp(nullptr),
			maxBucketCount(0u),
			size(0u),
			loadThreshold(0u),
			maxLoadFactor(0.5f)
		{}

		Impl(Impl&& other) noexcept :
			values(other.values),
			keys(other.keys),
			lookUp(other.lookUp),
			maxBucketCount(other.maxBucketCount),
			size(other.size),
			loadThreshold(other.loadThreshold),
			maxLoadFactor(other.maxLoadFactor)
		{
			other.maxBucketCount = 0u;
		}

		Impl(const Impl& other) :
			HasherWrapper(static_cast<const HasherWrapper&>(other)),
			EqualToWrapper(static_cast<const EqualToWrapper&>(other)),
			ValueAllocatorWrapper(static_cast<const ValueAllocatorWrapper&>(other)),
			KeyAllocatorWrapper(static_cast<const KeyAllocatorWrapper&>(other)),
			lookUpAllocatorWrapper(static_cast<const lookUpAllocatorWrapper&>(other))
		{
			copyConstruct(other);
		}

		~Impl() noexcept
		{
			destruct();
		}

		void operator=(Impl&& other)
		{
			destruct();

			static_cast<HasherWrapper&>(*this) = static_cast<HasherWrapper&&>(std::move(other));
			static_cast<EqualToWrapper&>(*this) = static_cast<EqualToWrapper&&>(std::move(other));
			static_cast<ValueAllocatorWrapper&>(*this) = static_cast<ValueAllocatorWrapper&&>(std::move(other));
			static_cast<KeyAllocatorWrapper&>(*this) = static_cast<KeyAllocatorWrapper&&>(std::move(other));
			static_cast<lookUpAllocatorWrapper&>(*this) = static_cast<lookUpAllocatorWrapper&&>(std::move(other));
			values = other.values;
			keys = other.keys;
			lookUp = other.lookUp;
			maxBucketCount = other.maxBucketCount;
			size = other.size;
			loadThreshold = other.loadThreshold;
			maxLoadFactor = other.maxLoadFactor;

			other.maxBucketCount = (size_type)0u;
		}

		void operator=(const Impl& other)
		{
			destruct();

			static_cast<HasherWrapper&>(*this) = static_cast<const HasherWrapper&>(other);
			static_cast<EqualToWrapper&>(*this) = static_cast<const EqualToWrapper&>(other);
			static_cast<ValueAllocatorWrapper&>(*this) = static_cast<const ValueAllocatorWrapper&>(other);
			static_cast<KeyAllocatorWrapper&>(*this) = static_cast<const KeyAllocatorWrapper&>(other);
			static_cast<lookUpAllocatorWrapper&>(*this) = static_cast<const lookUpAllocatorWrapper&>(other);
			copyConstruct(other);
		}
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
				new(&newKeys[i]) key_type{ std::move(impl.keys[i]) };
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

	static void swapOutElement(size_type previousBucket, size_type bucketIndex, Node* lookUp, size_type mask)
	{
		while (lookUp[bucketIndex].distanceFromIdealPosition > 1u)
		{
			const size_type newDistance = lookUp[bucketIndex].distanceFromIdealPosition - 1u;
			lookUp[previousBucket].distanceFromIdealPosition = newDistance;
			lookUp[previousBucket].indexInData = lookUp[bucketIndex].indexInData;

			previousBucket = bucketIndex;
			bucketIndex = (previousBucket + 1u) & mask;
		}
		lookUp[previousBucket].distanceFromIdealPosition = 0u;
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
	returns size if not found
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
		return impl.size;
	}
public:
	FastIterationHashMap() noexcept = default;

	FastIterationHashMap(FastIterationHashMap&&) noexcept = default;

	FastIterationHashMap(const FastIterationHashMap&) = default;

	~FastIterationHashMap() noexcept = default;

	void operator=(FastIterationHashMap&& other)
	{
		impl = std::move(other.impl);
	}

	void operator=(const FastIterationHashMap& other)
	{
		impl = other.impl;
	}

	iterator begin() noexcept
	{
		return impl.values;
	}

	const_iterator begin() const noexcept
	{
		return impl.values;
	}

	const_iterator cbegin() const noexcept
	{
		return impl.values;
	}

	iterator end() noexcept
	{
		return impl.values + impl.size;
	}

	const_iterator end() const noexcept
	{
		return impl.values + impl.size;
	}

	const_iterator cend() const noexcept
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
		new(&impl.keys[dataIndex]) key_type{ std::move(key) };
		new(&impl.values[dataIndex]) value_type{ std::move(value) };
		swapInElement(dataIndex, bucketIndex, impl.lookUp, impl.maxBucketCount - 1u);
	}

	void insert(const key_type& key, value_type&& value)
	{
		if (impl.size == impl.loadThreshold)
		{
			auto nextMaxBucketCountAndLoadThreshold = nextCapacity(impl.maxBucketCount, impl.loadThreshold, impl.maxLoadFactor);
			rehashNoChecks(nextMaxBucketCountAndLoadThreshold.maxBucketCount, nextMaxBucketCountAndLoadThreshold.loadThreshold);
		}
		const size_type dataIndex = impl.size;
		const size_type bucketIndex = (size_type)(impl.hash(key) & (impl.maxBucketCount - 1u));
		++impl.size;
		new(&impl.keys[dataIndex]) key_type{ key };
		new(&impl.values[dataIndex]) value_type{ std::move(value) };
		swapInElement(dataIndex, bucketIndex, impl.lookUp, impl.maxBucketCount - 1u);
	}

	void insert(key_type&& key, const value_type& value)
	{
		if (impl.size == impl.loadThreshold)
		{
			auto nextMaxBucketCountAndLoadThreshold = nextCapacity(impl.maxBucketCount, impl.loadThreshold, impl.maxLoadFactor);
			rehashNoChecks(nextMaxBucketCountAndLoadThreshold.maxBucketCount, nextMaxBucketCountAndLoadThreshold.loadThreshold);
		}
		const size_type dataIndex = impl.size;
		const size_type bucketIndex = (size_type)(impl.hash(key) & (impl.maxBucketCount - 1u));
		++impl.size;
		new(&impl.keys[dataIndex]) key_type{ std::move(key) };
		new(&impl.values[dataIndex]) value_type{ value };
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
		new(&impl.keys[dataIndex]) key_type{ key };
		new(&impl.values[dataIndex]) value_type{ value };
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
			impl.values[indexInData] = std::move(impl.values[size]);
			impl.keys[indexInData] = std::move(impl.keys[size]);
		}
		impl.values[size].~value_type();
		impl.keys[size].~key_type();

		swapOutElement(previousBucket, bucket, impl.lookUp, mask);
	}

	/*
	Cannot be called if the element isn't present
	*/
	value_type eraseAndGet(const key_type& key)
	{
		/**
		* Shift items back into the new empty space until we reach an empty space or an item in its ideal position.
		*/
		size_type previousBucket = findLookUpIndex(key);
		assert(previousBucket != std::numeric_limits<size_type>::max());

		const auto mask = impl.maxBucketCount - (size_type)1u;
		size_type bucket = (previousBucket + (size_type)1u) & mask;

		const auto indexInData = impl.lookUp[previousBucket].indexInData;
		value_type retVal = std::move(impl.values[indexInData]);
		--impl.size;
		const auto size = impl.size;
		if (indexInData != size)
		{
			impl.values[indexInData] = std::move(impl.values[size]);
			impl.keys[indexInData] = std::move(impl.keys[size]);
		}
		impl.values[size].~value_type();
		impl.keys[size].~key_type();

		swapOutElement(previousBucket, bucket, impl.lookUp, mask);
		return retVal;
	}

	iterator find(const key_type& key)
	{
		return impl.values + findIndexInData(key);
	}
};