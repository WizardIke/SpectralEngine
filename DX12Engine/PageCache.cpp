#include "PageCache.h"
#include <algorithm>
#include <iterator>
#include <cassert>
#include "PageAllocator.h"
#include "PageDeleter.h"

void PageCache::moveNodeToFront(Node* node)
{
	node->next->previous = node->previous;
	node->previous->next = node->next;

	node->next = mFront.next;
	node->previous = &mFront;
	mFront.next->previous = node;
	mFront.next = node;
}

PageAllocationInfo* PageCache::getPage(const TextureLocation& location)
{
	auto itr = pageLookUp.find(location);
	if (itr != pageLookUp.end())
	{
		moveNodeToFront(&*itr);
		return &itr->data;
	}
	return nullptr;
}

void PageCache::increaseSize(std::size_t newMaxPages)
{
	assert(newMaxPages >= maxPages && "increasing size to a smaller size");
	maxPages = newMaxPages;
}

PageCache::PageCache() : pageLookUp()
{
	mFront.next = &mBack;
	mBack.previous = &mFront;
}

PageCache::Node::Node(Node&& other) noexcept
{
	data = other.data;
	next = other.next;
	previous = other.previous;

	next->previous = this;
	previous->next = this;
}

void PageCache::Node::operator=(Node&& other) noexcept
{
	this->~Node();
	new(this) Node(std::move(other));
}

void PageCache::addPages(PageAllocationInfo* pageInfos, std::size_t pageCount, PageDeleter& pageDeleter)
{
	assert(maxPages != 0u);

	for (std::size_t i = 0u; i != pageCount; ++i)
	{
		const auto size = pageLookUp.size();
		if (size == maxPages)
		{
			Node* backPrev = mBack.previous;
			mBack.previous = backPrev->previous;
			mBack.previous->next = &mBack;
			pageDeleter.deletePage(backPrev->data);
			pageLookUp.erase(backPrev->data.textureLocation);
		}
		assert(size <= maxPages);
		Node newPage;
		newPage.data = pageInfos[i];
		newPage.next = mFront.next;
		newPage.previous = &mFront;
		//both of these are nesesary if no moves happen or the hashmap resizes before moving the newPage
		mFront.next->previous = &newPage;
		mFront.next = &newPage;

		pageLookUp.insert(std::move(newPage));
	}
}

void PageCache::decreaseSize(std::size_t newMaxPages, PageDeleter& pageDeleter)
{
	assert(newMaxPages < maxPages);
	const auto size = pageLookUp.size();
	if (size > newMaxPages)
	{
		const std::size_t numPagesToDelete = size - newMaxPages;
		for (std::size_t i = 0u; i != numPagesToDelete; ++i)
		{
			Node* backPrev = mBack.previous;
			mBack.previous = backPrev->previous;
			mBack.previous->next = &mBack;
			pageDeleter.deletePage(backPrev->data);
			pageLookUp.erase(backPrev->data.textureLocation);
		}
	}
	maxPages = newMaxPages;
}

void PageCache::removePageWithoutDeleting(const TextureLocation& location)
{
	auto page = pageLookUp.find(location);
	assert(page != pageLookUp.end() && "Cannot delete a page that doesn't exist");
	page->previous->next = page->next;
	page->next->previous = page->previous;
	pageLookUp.erase(page);
}