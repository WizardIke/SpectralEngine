#pragma once
#include <cstdint>
#include <robin_hash.h>
#include "TextureResitency.h"
#include "Range.h"
#include <vector>
#undef min;
#undef max;
class PageAllocator;

class PageCache
{
	struct Node
	{
		PageAllocationInfo data;
		Node* next, *previous;

		Node() = default;

		Node(Node&& other)
		{
			data = other.data;
			next = other.next;
			previous = other.previous;

			next->previous = this;
			previous->next = this;
		}

		void operator=(Node&& other)
		{
			this->~Node();
			new(this) Node(std::move(other));
		}
	};

	struct Hash : std::hash<uint64_t>
	{
		size_t operator()(textureLocation location)
		{
			return (*(std::hash<uint64_t>*)(this))(location.value);
		}
	};

	struct KeyEqual
	{
		bool operator()(textureLocation location1, textureLocation location2) const
		{
			return location1.value == location2.value;
		}
	};

	class KeySelect {
	public:
		using key_type = textureLocation;

		const key_type& operator()(const Node& key_value) const noexcept {
			return key_value.data.textureLocation;
		}

		key_type& operator()(Node& key_value) const noexcept {
			return key_value.data.textureLocation;
		}
	};

	class ValueSelect {
	public:
		using value_type = Node;

		const value_type& operator()(const Node& key_value) const noexcept {
			return key_value;
		}

		value_type& operator()(Node& key_value) const noexcept {
			return key_value;
		}
	};

	using HashMap = tsl::detail_robin_hash::robin_hash<Node, KeySelect, ValueSelect,
		Hash, KeyEqual, std::allocator<Node>, false, tsl::rh::power_of_two_growth_policy<2>>;

	Node mFront;
	Node mBack;
	HashMap pageLookUp;// this could be an std::unordered_map but the textureLocation would have to by duplicated in key and value; or getPage would have to return a pair
	size_t maxPages = 0u;

	void moveNodeToFront(Node* node);
	static const PageAllocationInfo& getDataFromNode(const Node& node) { return node.data; }
public:
	PageCache() : pageLookUp(HashMap::DEFAULT_INIT_BUCKETS_SIZE, Hash(), KeyEqual(), std::allocator<Node>(), HashMap::DEFAULT_MAX_LOAD_FACTOR) {}
	PageAllocationInfo* getPage(const textureLocation& location);
	size_t capacity() { return maxPages; }
	void increaseSize(std::size_t newMaxPages);

	/*
	 * adds pages removing the least recently used pages in the cache to make room. 
	 * adding pages to a cache with zero max size is undefined behavior.
	 */
	template<class PageDeleter>
	void addPages(PageAllocationInfo* pageInfos, size_t pageCount, PageDeleter& pageDeleter)
	{
		for (auto i = 0u; i < pageCount; ++i)
		{
			if (pageLookUp.size() == maxPages)
			{
				Node* back = mBack.previous;
				mBack.previous = back->previous;
				pageDeleter(back->data);
				pageLookUp.erase(back->data.textureLocation);
			}
			assert(pageLookUp.size() <= maxPages);
			Node newPage;
			newPage.data = pageInfos[i];
			newPage.next = mFront.next;
			newPage.previous = &mFront;
			mFront.next = &(pageLookUp.insert(newPage).first.value());
			if (pageLookUp.size() == 1u) mBack.previous = mFront.next;
		}
	}

	template<class PageDeleter>
	void decreaseSize(std::size_t newMaxPages, PageDeleter& pageDeleter)
	{
		assert(newMaxPages < maxPages);
		//pageList.decreaseSize(maxPages, newMaxPages);
		if (pageLookUp.size() > newMaxPages)
		{
			size_t numPagesToDelete = maxPages - newMaxPages;
			for (auto i = 0u; i != numPagesToDelete; ++i)
			{
				pageDeleter(mBack.previous->data);
				textureLocation oldBack = mBack.previous->data.textureLocation;
				mBack.previous = mBack.previous->previous;
				pageLookUp.erase(oldBack);
			}
		}
		maxPages = newMaxPages;
	}

	auto pages() -> decltype(std::declval<Range<decltype(pageLookUp.begin()), decltype(pageLookUp.end())>>().map<const PageAllocationInfo, getDataFromNode>())
	{
		Range<decltype(pageLookUp.begin()), decltype(pageLookUp.end())> rng = range(pageLookUp.begin(), pageLookUp.end());
		return rng.map<const PageAllocationInfo, getDataFromNode>();
	}

	void removePageWithoutDeleting(const textureLocation& location)
	{
		pageLookUp.erase(location);
	}
};
