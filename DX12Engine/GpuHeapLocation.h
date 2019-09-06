#pragma once

struct GpuHeapLocation
{
	unsigned long heapIndex;
	unsigned short heapOffsetInPages;

	GpuHeapLocation() {} //so containers don't zero heapIndex and heapOffsetInPages when they doen't need to
	GpuHeapLocation(unsigned long heapIndex, unsigned short heapOffsetInPages) : heapIndex(heapIndex), heapOffsetInPages(heapOffsetInPages) {} //needed because of other constructor
};