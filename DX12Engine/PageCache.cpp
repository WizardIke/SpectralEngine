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

PageAllocationInfo* PageCache::getPage(PageResourceLocation location, VirtualTextureInfo& textureInfo)
{
	auto iter = textureInfo.pageCacheData.pageLookUp.find(location);
	if (iter == textureInfo.pageCacheData.pageLookUp.end())
	{
		return nullptr;
	}
	moveNodeToFront(&*iter);
	return &iter->data;
}

void PageCache::increaseCapacity(std::size_t newMaxPages)
{
	assert(newMaxPages >= maxPages && "increasing size to a smaller size");
	maxPages = newMaxPages;
}

PageCache::PageCache()
{
	mFront.next = &mBack;
	mBack.previous = &mFront;
}

void PageCache::addPage(PageAllocationInfo pageInfo, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter)
{
	assert(maxPages != 0u);
	if(mSize == maxPages)
	{
		Node* backPrev = mBack.previous;
		mBack.previous = backPrev->previous;
		mBack.previous->next = &mBack;
		VirtualTextureInfo& pageToRemoveTextureInfo = texturesById[backPrev->data.textureLocation.textureId];
		if (backPrev->data.heapLocation.heapOffsetInPages == std::numeric_limits<unsigned int>::max())
		{
			++pageToRemoveTextureInfo.pageCacheData.numberOfUnneededLoadingPages;
			pageDeleter.deletePage(backPrev->data.textureLocation, texturesById);
		}
		else
		{
			pageDeleter.deletePage(backPrev->data, texturesById);
		}
		pageToRemoveTextureInfo.pageCacheData.pageLookUp.erase(backPrev->data.textureLocation);
	}
	else
	{
		++mSize;
	}
	Node newPage;
	newPage.data = pageInfo;
	newPage.next = mFront.next;
	newPage.previous = &mFront;
	//both of these are nesesary if no moves happen or the hashmap resizes before moving the newPage
	mFront.next->previous = &newPage;
	mFront.next = &newPage;

	textureInfo.pageCacheData.pageLookUp.insert(std::move(newPage));
}

void PageCache::addNonAllocatedPage(PageResourceLocation location, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter)
{
	addPage({{std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max()}, location}, textureInfo, texturesById, pageDeleter);
}

void PageCache::decreaseCapacity(std::size_t newMaxPages, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter)
{
	assert(newMaxPages < maxPages);
	if (mSize > newMaxPages)
	{
		const std::size_t numPagesToDelete = mSize - newMaxPages;
		mSize = newMaxPages;
		for (std::size_t i = 0u; i != numPagesToDelete; ++i)
		{
			Node* backPrev = mBack.previous;
			mBack.previous = backPrev->previous;
			mBack.previous->next = &mBack;
			VirtualTextureInfo& pageToRemoveTextureInfo = texturesById[backPrev->data.textureLocation.textureId];
			if(backPrev->data.heapLocation.heapOffsetInPages == std::numeric_limits<unsigned int>::max())
			{
				++pageToRemoveTextureInfo.pageCacheData.numberOfUnneededLoadingPages;
				pageDeleter.deletePage(backPrev->data.textureLocation, texturesById);
			}
			else
			{
				pageDeleter.deletePage(backPrev->data, texturesById);
			}
			pageToRemoveTextureInfo.pageCacheData.pageLookUp.erase(backPrev->data.textureLocation);
		}
	}
	maxPages = newMaxPages;
}

void PageCache::removePage(PageResourceLocation location, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter)
{
	auto page = textureInfo.pageCacheData.pageLookUp.find(location);
	assert(page != textureInfo.pageCacheData.pageLookUp.end() && "Cannot delete a page that doesn't exist");
	page->previous->next = page->next;
	page->next->previous = page->previous;
	textureInfo.pageCacheData.pageLookUp.erase(page);
	pageDeleter.deletePage(location, texturesById); //remove page from resource
}

bool PageCache::containsDoNotMarkAsRecentlyUsed(PageResourceLocation location, VirtualTextureInfo& textureInfo)
{
	return textureInfo.pageCacheData.pageLookUp.contains(location);
}

bool PageCache::contains(PageResourceLocation location, VirtualTextureInfo& textureInfo)
{
	auto iter = textureInfo.pageCacheData.pageLookUp.find(location);
	if (iter == textureInfo.pageCacheData.pageLookUp.end())
	{
		return false;
	}
	moveNodeToFront(&*iter);
	return true;
}

void PageCache::setPageAsAllocated(PageResourceLocation location, VirtualTextureInfo& textureInfo, GpuHeapLocation newHeapLocation)
{
	textureInfo.pageCacheData.pageLookUp.find(location)->data.heapLocation = newHeapLocation;
}

void PageCache::removePage(Node* page)
{
	page->previous->next = page->next;
	page->next->previous = page->previous;
}