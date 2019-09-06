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

	node->next = mNodes.next;
	node->previous = &mNodes;
	mNodes.next->previous = node;
	mNodes.next = node;
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
	assert(newMaxPages >= maxPages && "increasing capacity to a smaller capacity");
	maxPages = newMaxPages;
}

PageCache::PageCache()
{
	mNodes.next = &mNodes;
	mNodes.previous = &mNodes;
}

void PageCache::addPage(PageAllocationInfo pageInfo, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter)
{
	assert(maxPages != 0u);
	if(mSize == maxPages)
	{
		Node* backPrev = mNodes.previous;
		mNodes.previous = backPrev->previous;
		mNodes.previous->next = &mNodes;
		VirtualTextureInfo& pageToRemoveTextureInfo = texturesById[backPrev->data.textureLocation.textureId];
		if (backPrev->data.heapLocation.heapOffsetInPages == std::numeric_limits<decltype(backPrev->data.heapLocation.heapOffsetInPages)>::max())
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
	newPage.next = mNodes.next;
	newPage.previous = &mNodes;
	//both of these are nesesary if the hashmap resizes before moving the newPage
	mNodes.next->previous = &newPage;
	mNodes.next = &newPage;

	textureInfo.pageCacheData.pageLookUp.insert(std::move(newPage));
}

void PageCache::addNonAllocatedPage(PageResourceLocation location, VirtualTextureInfo& textureInfo, VirtualTextureInfoByID& texturesById, PageDeleter& pageDeleter)
{
	addPage({{std::numeric_limits<decltype(std::declval<GpuHeapLocation>().heapIndex)>::max(), std::numeric_limits<decltype(std::declval<GpuHeapLocation>().heapOffsetInPages)>::max()}, location},
		textureInfo, texturesById, pageDeleter);
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
			Node* backPrev = mNodes.previous;
			mNodes.previous = backPrev->previous;
			mNodes.previous->next = &mNodes;
			VirtualTextureInfo& pageToRemoveTextureInfo = texturesById[backPrev->data.textureLocation.textureId];
			if(backPrev->data.heapLocation.heapOffsetInPages == std::numeric_limits<decltype(backPrev->data.heapLocation.heapOffsetInPages)>::max())
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
	auto page = textureInfo.pageCacheData.pageLookUp.find(location);
	assert(page != textureInfo.pageCacheData.pageLookUp.end() && "setting page as allocated that isn't in the texture");
	page->data.heapLocation = newHeapLocation;
}

void PageCache::removePage(Node* page)
{
	page->previous->next = page->next;
	page->next->previous = page->previous;
}