#include "PageCachePerTextureData.h"

PageCachePerTextureData::Node::Node(Node&& other) noexcept :
	data(other.data),
	next(other.next),
	previous(other.previous)
{
	next->previous = this;
	previous->next = this;
}

void PageCachePerTextureData::Node::operator=(Node&& other) noexcept
{
	data = other.data;
	next = other.next;
	previous = other.previous;

	next->previous = this;
	previous->next = this;
}