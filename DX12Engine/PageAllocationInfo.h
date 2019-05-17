#pragma once
#include "GpuHeapLocation.h"
#include "PageResourceLocation.h"

struct PageAllocationInfo
{
	GpuHeapLocation heapLocation;
	PageResourceLocation textureLocation;
};