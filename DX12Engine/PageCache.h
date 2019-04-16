#pragma once
#include <cstdint>
#undef min
#undef max
#include "HashSet.h"
#include "PageAllocationInfo.h"
#include "Range.h"
#include <cassert>
#undef min
#undef max
class PageDeleter;

class PageCache
{
	struct Node
	{
		PageAllocationInfo data;
		Node* next, *previous;

		Node() = default;
		Node(Node&& other) noexcept;
		void operator=(Node&& other) noexcept;
	};

	struct Hash : std::hash<uint64_t>
	{
		std::size_t operator()(const Node& node) const
		{
			return (*(std::hash<uint64_t>*)(this))(node.data.textureLocation.value);
		}

		std::size_t operator()(TextureLocation location) const
		{
			return (*(std::hash<uint64_t>*)(this))(location.value);
		}
	};

	struct KeyEqual
	{
		bool operator()(const Node& node, TextureLocation location1) const
		{
			return node.data.textureLocation.value == location1.value;
		}

		bool operator()(const Node& node1, const Node& node2) const
		{
			return node1.data.textureLocation.value == node2.data.textureLocation.value;
		}
	};

	Node mFront;
	Node mBack;
	HashSet<Node, Hash, KeyEqual> pageLookUp;// this could be an std::unordered_map but the textureLocation would have to by duplicated in key and value; or getPage would have to return a pair
	std::size_t maxPages = 0u;

	void moveNodeToFront(Node* node);
	static const PageAllocationInfo& getDataFromNode(const Node& node) { return node.data; }
public:
	PageCache();
	/* gets a page and marks it as the most recently used */
	PageAllocationInfo* getPage(const TextureLocation& location);
	std::size_t capacity() const { return maxPages; }
	std::size_t size() const { return pageLookUp.size(); }
	void increaseSize(std::size_t newMaxPages);

	/*
	* adds pages removing the least recently used pages in the cache to make room.
	* adding pages to a cache with zero max size is undefined behavior.
	*/
	void addPages(const PageAllocationInfo* pageInfos, std::size_t pageCount, PageDeleter& pageDeleter);

	void addPage(const PageAllocationInfo& pageInfo, PageDeleter& pageDeleter);

	void addNonAllocatedPage(const TextureLocation& location, PageDeleter& pageDeleter);

	bool contains(TextureLocation location);
	void setPageAsAllocated(TextureLocation location, HeapLocation newHeapLocation);

	void decreaseSize(std::size_t newMaxPages, PageDeleter& pageDeleter);

	//should try linked list iterator
	auto pages() const -> decltype(std::declval<Range<decltype(this->pageLookUp.begin()), decltype(this->pageLookUp.end())>>().map<const PageAllocationInfo&, &getDataFromNode>())
	{
		return range(pageLookUp).map<const PageAllocationInfo&, &getDataFromNode>();
	}

	/* Removes a page without freeing heap space */
	void removePage(const TextureLocation& location, PageDeleter& pageDeleter);
};