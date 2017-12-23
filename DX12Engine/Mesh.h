#pragma once
#include "D3D12Resource.h"
#include "D3D12Heap.h"

struct Mesh
{
	D3D12Resource vertices;
	D3D12Resource indices;
	D3D12Heap buffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	uint32_t indexCount;
};