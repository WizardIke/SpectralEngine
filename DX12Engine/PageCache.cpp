#include "PageCache.h"
#include <algorithm>
#include <iterator>
#include <cassert>
#include "PageAllocator.h"

void PageCache::moveNodeToFront(Node* node)
{
	if (node != mFront.next)
	{
		if (node == mBack.previous)
		{
			mBack.previous = node->previous;
		}
		else
		{
			node->next->previous = node->previous;
			node->previous->next = node->next;
		}
		node->next = mFront.next;
		mFront.next = node;
	}
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
	assert(newMaxPages > maxPages);
	maxPages = newMaxPages;
}