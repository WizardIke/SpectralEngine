#pragma once
#include "Mesh.h"
#include <memory>
#pragma warning (disable : 4624)

class FixedSizeAllocator
{
	constexpr static size_t slabSize = 32u;
	union Element {
		Mesh mesh;
		Element* nextFreeMesh;
	};
	char* currentSlab;
	Element* currentFreeElement;
	Element* lastFreeElement;

	void allocateSlab();
public:
	FixedSizeAllocator();
	~FixedSizeAllocator();

	Mesh* allocate();
	void deallocate(Mesh* mesh);
};