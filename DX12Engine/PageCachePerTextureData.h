#pragma once
#include "PageResourceLocation.h"
#include "PageAllocationInfo.h"
#include <cstdint> //uint64_t
#include "HashSet.h"

class PageCachePerTextureData
{
public:
	struct Node
	{
		PageAllocationInfo data;
		Node* next, *previous;

		Node() = default;
		Node(Node&& other) noexcept;
		void operator=(Node&& other) noexcept;
	};
private:
	struct Hash : private PageResourceLocation::HashNoTextureId
	{
		std::size_t operator()(const Node& node) const
		{
			return (*this)(node.data.textureLocation);
		}

		using PageResourceLocation::HashNoTextureId::operator();
	};

	struct KeyEqual
	{
		bool operator()(const Node& node, PageResourceLocation location1) const
		{
			return node.data.textureLocation.mipLevel == location1.mipLevel && node.data.textureLocation.x == location1.x && node.data.textureLocation.y == location1.y;
		}

		bool operator()(const Node& node1, const Node& node2) const
		{
			return (*this)(node1, node2.data.textureLocation);
		}
	};
public:
	//TODO: Need to tell the hashSet that the hash of a key is unique
	HashSet<Node, Hash, KeyEqual> pageLookUp; //this could be an std::unordered_map but the textureLocation would have to be duplicated in key and value.
};