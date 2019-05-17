#pragma once

struct GpuHeapLocation
{
	unsigned int heapIndex;
	unsigned int heapOffsetInPages;

	GpuHeapLocation() {} //so containers don't zero heapIndex and heapOffsetInPages when they doen't need to
	GpuHeapLocation(unsigned int heapIndex, unsigned int heapOffsetInPages) : heapIndex(heapIndex), heapOffsetInPages(heapOffsetInPages) {} //needed because of other constructor
};