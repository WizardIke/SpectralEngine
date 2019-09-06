#pragma once
#include <cstdint>
#undef min
#undef max
#include "HashSet.h"
#include "GpuHeapLocation.h"
#include "PageAllocationInfo.h"
#include "VirtualTextureInfo.h"
#include "VirtualTextureInfoByID.h"
#include "PageCachePerTextureData.h"
#include "Range.h"
#include <cassert>
#undef min
#undef max
class PageDeleter;

class PageCache
{
	using Node = PageCachePerTextureData::Node;
	Node mNodes;
	std::size_t mSize = 0u;
	std::size_t maxPages = 0u;

	void moveNodeToFront(Node* node);
	static const PageAllocationInfo& getDataFromNode(const Node& node) { return node.data; }

	class IteratorBase
	{
		friend class PageCache;
	private:
		Node* current;
		IteratorBase(Node* start) :
			current(start)
		{}
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = PageAllocationInfo;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		IteratorBase& operator++() 
		{ 
			current = current->next;
			return *this; 
		}
		IteratorBase& operator--() 
		{ 
			current = current->previous; 
			return *this; 
		}
		IteratorBase operator++(int)
		{ 
			IteratorBase r(*this);
			++(*this); return r; 
		}
		IteratorBase operator--(int) 
		{ 
			IteratorBase r(*this);
			--(*this); return r; 
		}

		bool operator==(IteratorBase const& rhs) const
		{
			return current == rhs.current; 
		}
		bool operator!=(IteratorBase const& rhs) const
		{
			return current != rhs.current; 
		}
	};

	class ConstIterator : public IteratorBase
	{
		friend class PageCache;
		ConstIterator(Node* st) : IteratorBase(st) {}
	public:
		const value_type& operator*() const
		{ 
			return current->data;
		}
	};

	class Iterator : public IteratorBase
	{
		friend class PageCache;
		Iterator(Node* st) : IteratorBase(st) {}
	public:
		Iterator(const ConstIterator& constIterator) : IteratorBase(constIterator.current) {}

		value_type& operator*()
		{
			return current->data;
		}
	};
public:
	using iterator = Iterator;
	using const_iterator = ConstIterator;

	PageCache();
	/* gets a page and marks it as the most recently used */
	PageAllocationInfo* getPage(PageResourceLocation location, VirtualTextureInfo& textureInfo);
	std::size_t capacity() const { return maxPages; }
	std::size_t size() const { return mSize; }
	void increaseCapacity(std::size_t newMaxPages);

	/*
	* adds a page removing the least recently used page in the cache to make room.
	* adding a page to a cache with zero max size is undefined behavior.
	*/
	void addPage(PageAllocationInfo pageInfo, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter);

	void addNonAllocatedPage(PageResourceLocation location, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter);

	/*
	* Checks if the page is in the cache but doesn't mark it as most recently used.
	*/
	bool containsDoNotMarkAsRecentlyUsed(PageResourceLocation location, VirtualTextureInfo& textureInfo);

	bool contains(PageResourceLocation location, VirtualTextureInfo& textureInfo);

	void setPageAsAllocated(PageResourceLocation location, VirtualTextureInfo& textureInfo, GpuHeapLocation newHeapLocation);

	void decreaseCapacity(std::size_t newMaxPages, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter);

	Range<const_iterator, const_iterator> pages() const
	{
		return range(const_iterator(mNodes.next), const_iterator(const_cast<Node*>(&mNodes)));
	}

	Range<iterator, iterator> pages()
	{
		return range(iterator(mNodes.next), iterator(&mNodes));
	}

	/* Removes a page without freeing ID3D12Heap space */
	void removePage(PageResourceLocation location, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter);

	/* Removes a page from the cache without deleting it */
	void removePage(Node* page);
};