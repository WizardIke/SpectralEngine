#pragma once
#include "Mesh.h"
#include <memory>
#pragma warning (disable : 4624) // warns about Elements destructor being implicity deleted, but this is fine

/*
allocates and deallocates memory of a fixed size and alignment.
memory allocated with this FixedSizeAllocator can by deallocated with another, but if the other FixedSizeAllocator goes out of scope use of memory allocated by this FixedSizeAllocator is undefined.
when a FixedSizeAllocator goes out of scope all memory allocated by it is deallocated.
*/
template<class T, size_t slabSize = 32u>
class FixedSizeAllocator
{
public:
	using value_type = T;
private:
	union Element {
		value_type mesh;
		Element* nextFreeMesh;
	};
	char* currentSlab;
	Element* currentFreeElement;
	Element* lastFreeElement;

	void allocateSlab()
	{
		size_t size;
		char* nextSlab;
		if (alignof(Element) > alignof(char*))
		{
			size = sizeof(char*) + (alignof(Element)-alignof(char*)) + sizeof(Element) * slabSize;
			nextSlab = (char*)malloc(size);
			currentFreeElement = reinterpret_cast<Element*>(((reinterpret_cast<std::uintptr_t>(nextSlab) + sizeof(char*)) + alignof(Element)-1) & (alignof(Element)-1));
		}
		else
		{
			size = sizeof(char*) + sizeof(Element) * slabSize;
			nextSlab = (char*)malloc(size);
			currentFreeElement = reinterpret_cast<Element*>(nextSlab + sizeof(char*));
		}
		reinterpret_cast<char**>(nextSlab)[0] = currentSlab;
		currentSlab = nextSlab;
		Element* end = currentFreeElement + slabSize - 1u;
		for (auto start = currentFreeElement;;)
		{
			if (start == end)
			{
				start->nextFreeMesh = nullptr;
				lastFreeElement = start;
				break;
			}
			Element* next = start + 1;
			start->nextFreeMesh = next;
			start = next;
		}
	}
public:
	FixedSizeAllocator() : currentSlab(nullptr), currentFreeElement(nullptr) {}

	~FixedSizeAllocator()
	{
		auto slab = currentSlab;
		while (slab)
		{
			free(slab);
			slab = reinterpret_cast<char**>(slab)[0];
		}
	}

	value_type* allocate()
	{
		if (!currentFreeElement) { allocateSlab(); }

		Mesh* returnMesh = &(currentFreeElement->mesh);
		currentFreeElement = currentFreeElement->nextFreeMesh;
		return returnMesh;
	}

	void deallocate(value_type* mesh)
	{
		lastFreeElement->nextFreeMesh = reinterpret_cast<Element*>(mesh);
		lastFreeElement = reinterpret_cast<Element*>(mesh);
		lastFreeElement->nextFreeMesh = nullptr;
	}
};