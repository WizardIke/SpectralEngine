#include "MeshAllocator.h"

void FixedSizeAllocator::allocateSlab()
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

FixedSizeAllocator::FixedSizeAllocator() : currentSlab(nullptr), currentFreeElement(nullptr) {}

FixedSizeAllocator::~FixedSizeAllocator()
{
	auto slab = currentSlab;
	while (slab)
	{
		free(slab);
		slab = reinterpret_cast<char**>(slab)[0];
	}
}

Mesh* FixedSizeAllocator::allocate()
{
	if (!currentFreeElement) { allocateSlab(); }

	Mesh* returnMesh = &(currentFreeElement->mesh);
	currentFreeElement = currentFreeElement->nextFreeMesh;
	//new (returnMesh) Mesh();
	return returnMesh;
}

void FixedSizeAllocator::deallocate(Mesh* mesh)
{
	//mesh->~Mesh();
	lastFreeElement->nextFreeMesh = reinterpret_cast<Element*>(mesh);
	lastFreeElement = reinterpret_cast<Element*>(mesh);
	lastFreeElement->nextFreeMesh = nullptr;
}