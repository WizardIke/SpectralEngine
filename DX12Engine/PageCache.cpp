#include "PageCache.h"
#include <algorithm>
#include <iterator>
#include <cassert>
#include "PageAllocator.h"

void PageCache::moveNodeToFront(Node* node)
{
	//if (node != mFront.next)
	//{
		node->next->previous = node->previous;
		node->previous->next = node->next;

		node->next = mFront.next;
		node->previous = &mFront;
		mFront.next->previous = node;
		mFront.next = node;
	//}
}

PageAllocationInfo* PageCache::getPage(const textureLocation& location)
{
	auto itr = pageLookUp.find(location);
	if (itr != pageLookUp.end())
	{
		moveNodeToFront(&itr.value());
		return &itr.value().data;
	}
	return nullptr;
}

void PageCache::increaseSize(size_t newMaxPages)
{
#ifndef NDEBUG
	if (newMaxPages < maxPages)
	{
		assert(!"increasing size to a smaller size");
	}
#endif // !NDEBUG
	maxPages = newMaxPages;
}

PageCache::PageCache() : pageLookUp(HashMap::DEFAULT_INIT_BUCKETS_SIZE, Hash(), KeyEqual(), std::allocator<Node>(), HashMap::DEFAULT_MAX_LOAD_FACTOR)
{
	mFront.next = &mBack;
	mBack.previous = &mFront;
}