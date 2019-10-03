#pragma once
#include <functional> //std::hash, std::equal_to
#include <cstddef> //std::size_t
#include <limits> //std::numeric_limits
#include <array>
#include <cassert>
#include <new> //placement new, std::launder
#include "FixedCapacityDynamicArray.h"

template<class Key, std::size_t baseCapacity, class Hasher = std::hash<Key>, class EqualTo = std::equal_to<>>
class FixedCapacityFastIterationHashSet
{
private:
	static constexpr std::size_t roundUpToPowerOf2(std::size_t v)
	{
		--v;
		constexpr auto numberOfBitsInSizeType = (std::size_t)(sizeof(std::size_t) * CHAR_BIT);
		for (std::size_t i = (std::size_t)1u; i != numberOfBitsInSizeType; i *= (std::size_t)2u)
		{
			v |= v >> i;
		}
		++v;
		return v;
	}

	constexpr static std::size_t maxBucketCount = (baseCapacity == 0u ? 0u : roundUpToPowerOf2(baseCapacity)) * 2u;
public:
	using key_type = Key;
	using value_type = Key;
	using size_type = std::conditional_t<baseCapacity <= std::numeric_limits<uint_least8_t>::max(), uint_least8_t,
		std::conditional_t<baseCapacity <= std::numeric_limits<uint_least16_t>::max(), uint_least16_t,
		std::conditional_t<baseCapacity <= std::numeric_limits<uint_least32_t>::max(), uint_least32_t,
		std::conditional_t<baseCapacity <= std::numeric_limits<uint_least64_t>::max(), uint_least64_t, std::size_t>>>>;
private:
	constexpr static size_type mCapacity = static_cast<size_type>(baseCapacity);
public:
	using difference_type = std::ptrdiff_t;
	using hasher = Hasher;
	using key_equal = EqualTo;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using iterator = typename FixedCapacityDynamicArray<key_type, mCapacity>::iterator;
	using const_iterator = typename FixedCapacityDynamicArray<key_type, mCapacity>::const_iterator;
private:
	struct Node
	{
		size_type distanceFromIdealPosition;
		size_type indexInData;
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

	struct Impl : private HasherWrapper, private EqualToWrapper
	{
		using HasherWrapper::hash;
		using EqualToWrapper::equal;

		using key_type = FixedCapacityFastIterationHashSet::key_type;
		using size_type = FixedCapacityFastIterationHashSet::size_type;
		using Node = FixedCapacityFastIterationHashSet::Node;
		constexpr static std::size_t maxBucketCount = FixedCapacityFastIterationHashSet::maxBucketCount;
		constexpr static size_type mCapacity = FixedCapacityFastIterationHashSet::mCapacity;

#pragma warning(suppress: 4324) //warns about padding due to alignment
		FixedCapacityDynamicArray<key_type, mCapacity> keys;
		std::array<Node, maxBucketCount> lookUp;
	public:
		Impl() noexcept :
			lookUp{}
		{}

		Impl(Impl&& other) noexcept :
			HasherWrapper(std::move(static_cast<const HasherWrapper&>(other))),
			EqualToWrapper(std::move(static_cast<const EqualToWrapper&>(other))),
			lookUp{ other.lookUp },
			keys{ std::move(other.keys) }
		{}

		Impl(const Impl& other) :
			HasherWrapper(static_cast<const HasherWrapper&>(other)),
			EqualToWrapper(static_cast<const EqualToWrapper&>(other)),
			lookUp{ other.lookUp },
			keys(other.keys)
		{}
	};
	Impl impl;

	static void swapInElement(size_type dataIndex, std::size_t bucketIndex, Node* lookUp)
	{
		static_assert(maxBucketCount != std::size_t{ 0u });
		constexpr std::size_t mask = maxBucketCount - static_cast<std::size_t>(1u);
		size_type distanceFromIdealBucket = (size_type)1u;
		while (true)
		{
			if (lookUp[bucketIndex].distanceFromIdealPosition == (size_type)0u)
			{
				lookUp[bucketIndex].indexInData = dataIndex;
				lookUp[bucketIndex].distanceFromIdealPosition = distanceFromIdealBucket;
				break;
			}
			else if (lookUp[bucketIndex].distanceFromIdealPosition < distanceFromIdealBucket)
			{
				std::swap(dataIndex, lookUp[bucketIndex].indexInData);
				std::swap(distanceFromIdealBucket, lookUp[bucketIndex].distanceFromIdealPosition);
			}

			bucketIndex = (bucketIndex + std::size_t{ 1u }) & mask;
			++distanceFromIdealBucket;
		}
	}

	static void swapOutElement(std::size_t previousBucket, std::size_t bucketIndex, Node* lookUp)
	{
		static_assert(maxBucketCount != std::size_t{ 0u });
		constexpr std::size_t mask = maxBucketCount - static_cast<std::size_t>(1u);
		while (lookUp[bucketIndex].distanceFromIdealPosition > (size_type)1u)
		{
			const size_type newDistance = lookUp[bucketIndex].distanceFromIdealPosition - (size_type)1u;
			lookUp[previousBucket].distanceFromIdealPosition = newDistance;
			lookUp[previousBucket].indexInData = lookUp[bucketIndex].indexInData;

			previousBucket = bucketIndex;
			bucketIndex = (previousBucket + static_cast<std::size_t>(1u)) & mask;
		}
		lookUp[previousBucket].distanceFromIdealPosition = (size_type)0u;
	}

	std::size_t findLookUpIndex(const key_type& key)
	{
		if constexpr (maxBucketCount == std::size_t{ 0u })
		{
			return std::numeric_limits<std::size_t>::max();
		}

		constexpr std::size_t mask = maxBucketCount - (std::size_t)1u;
		std::size_t bucketIndex = impl.hash(key) & mask;
		size_type distanceFromIdealBucket = size_type{ 1u };
		while (impl.lookUp[bucketIndex].distanceFromIdealPosition >= distanceFromIdealBucket)
		{
			const size_type indexInData = impl.lookUp[bucketIndex].indexInData;
			const auto& bucketKey = impl.keys[indexInData];
			if (impl.equal(bucketKey, key)) return bucketIndex;
			bucketIndex = (bucketIndex + (std::size_t)1u) & mask;
			++distanceFromIdealBucket;
		}
		return std::numeric_limits<std::size_t>::max();
	}

	struct FindOrCreateResult
	{
		std::size_t lookUpIndex;
		key_type* data;
	};

	template<class... Keys>
	FindOrCreateResult findOrGetLocationToCreate(const Keys&... keys)
	{
		static_assert(maxBucketCount != std::size_t{ 0u });

		constexpr std::size_t mask = maxBucketCount - std::size_t{ 1u };
		std::size_t bucketIndex = std::size_t{ impl.hash(keys...) } & mask;
		size_type distanceFromIdealBucket = size_type{ 1u };
		while (impl.lookUp[bucketIndex].distanceFromIdealPosition >= distanceFromIdealBucket)
		{
			const size_type indexInData = impl.lookUp[bucketIndex].indexInData;
			auto& bucketKey = impl.keys[indexInData];
			if (impl.equal(bucketKey, keys...)) return { bucketIndex, &bucketKey };
			bucketIndex = (bucketIndex + (std::size_t)1u) & mask;
			++distanceFromIdealBucket;
		}
		return { bucketIndex, nullptr };
	}

	/*
	returns size if not found
	*/
	size_type findIndexInData(const key_type& key) const
	{
		if constexpr (maxBucketCount == std::size_t{ 0u })
		{
			return (size_type)0u;
		}
		constexpr std::size_t mask = maxBucketCount - std::size_t{ 1u };
		std::size_t bucketIndex = impl.hash(key) & mask;
		size_type distanceFromIdealBucket = size_type{ 1u };
		while (impl.lookUp[bucketIndex].distanceFromIdealPosition >= distanceFromIdealBucket)
		{
			const size_type indexInData = impl.lookUp[bucketIndex].indexInData;
			const auto& bucketKey = impl.keys[indexInData];
			if (impl.equal(bucketKey, key)) return indexInData;
			bucketIndex = (bucketIndex + std::size_t{ 1u }) & mask;
			++distanceFromIdealBucket;
		}
		return impl.keys.size();
	}
public:
	FixedCapacityFastIterationHashSet() noexcept = default;

	FixedCapacityFastIterationHashSet(FixedCapacityFastIterationHashSet&&) noexcept = default;

	FixedCapacityFastIterationHashSet(const FixedCapacityFastIterationHashSet&) = default;

	~FixedCapacityFastIterationHashSet() noexcept = default;

	iterator begin() noexcept
	{
		return impl.keys.begin();
	}

	const_iterator begin() const noexcept
	{
		return impl.keys.begin();
	}

	const_iterator cbegin() const noexcept
	{
		return impl.keys.begin();
	}

	iterator end() noexcept
	{
		impl.keys.end();
	}

	const_iterator end() const noexcept
	{
		impl.keys.end();
	}

	const_iterator cend() const noexcept
	{
		impl.keys.end();
	}

	bool empty() const noexcept
	{
		return impl.keys.size() == (size_type)0u;
	}

	size_type size() const noexcept
	{
		return impl.keys.size();
	}

	static size_type capacity() noexcept
	{
		return mCapacity;
	}

	static size_type max_size() noexcept
	{
		return mCapacity;
	}

	void insert(key_type&& key)
	{
		assert(impl.keys.size() != mCapacity);
		const size_type dataIndex = impl.keys.size();
		constexpr auto mask = maxBucketCount - std::size_t{ 1u };
		const std::size_t bucketIndex = std::size_t{ impl.hash(key) } & mask;
		impl.keys.push_back(std::move(key));
		swapInElement(dataIndex, bucketIndex, impl.lookUp.data());
	}

	void insert(const key_type& key)
	{
		assert(impl.keys.size() != mCapacity);
		const size_type dataIndex = impl.keys.size();
		constexpr auto mask = maxBucketCount - std::size_t{ 1u };
		const std::size_t bucketIndex = std::size_t{ impl.hash(key) } & mask;
		impl.keys.push_back(key);
		swapInElement(dataIndex, bucketIndex, impl.lookUp.data());
	}

	void erase(const key_type& key)
	{
		/**
		* Shift items back into the new empty space until we reach an empty space or an item in its ideal position.
		*/
		std::size_t previousBucket = findLookUpIndex(key);
		if (previousBucket == std::numeric_limits<decltype(previousBucket)>::max())
		{
			return;
		}
		constexpr auto mask = maxBucketCount - (std::size_t)1u;
		std::size_t bucket = (previousBucket + (std::size_t)1u) & mask;

		const auto indexInData = impl.lookUp[previousBucket].indexInData;
		if (indexInData != impl.keys.size() - 1u)
		{
			impl.keys[indexInData] = std::move(impl.keys[impl.keys.size() - 1u]);
		}
		impl.keys.pop_back();

		swapOutElement(previousBucket, bucket, impl.lookUp.data());
	}

	iterator find(const key_type& key)
	{
		return impl.keys.begin() + findIndexInData(key);
	}

	const_iterator find(const key_type& key) const
	{
		return impl.keys.begin() + findIndexInData(key);
	}

	template<class F>
	void consume(F&& f)
	{
		if (impl.keys.size() == 0u)
		{
			return;
		}
		for (auto& value : impl.keys)
		{
			std::size_t previousBucket = findLookUpIndex(value);
			constexpr auto mask = maxBucketCount - (std::size_t)1u;
			std::size_t bucket = (previousBucket + (std::size_t)1u) & mask;
			swapOutElement(previousBucket, bucket, impl.lookUp.data());

			f(std::move(value));
		}
		impl.keys.clear();
	}
private:
	template<class KeyType>
	void keyAssignmentHelper(key_type* data, KeyType&& key)
	{
		*data = std::forward<KeyType>(key);
	}

	template<class... Keys>
	void keyAssignmentHelper(key_type* data, Keys&&... keys)
	{
		*data = key_type{ std::forward<Keys>(keys)... };
	}
public:
	template<class... Keys>
	bool insertOrReplace(Keys&&... keys)
	{
		auto result = findOrGetLocationToCreate(keys...);
		if (result.data == nullptr)
		{
			assert(impl.key.size() != mCapacity);
			const size_type dataIndex = impl.keys.size();
			impl.keys.emplace(std::forward<Keys>(keys)...);
			swapInElement(dataIndex, result.lookUpIndex, impl.lookUp.data());
			return true;
		}
		keyAssignmentHelper(result.data, std::forward<Keys>(keys)...);
		return false;
	}

	template<class... Keys>
	key_type& operator[](Keys&&... keys)
	{
		auto result = findOrGetLocationToCreate(keys...);
		if (result.data == nullptr)
		{
			assert(impl.key.size() != mCapacity);
			const size_type dataIndex = impl.key.size();
			impl.keys.emplace(std::forward<Keys>(keys)...);
			swapInElement(dataIndex, result.lookUpIndex, impl.lookUp.data());
			return *keyPtr;
		}
		return *result.data;
	}
};