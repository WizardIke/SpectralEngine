#pragma once
#include "D3D12Resource.h"
#include "D3D12Heap.h"

class Mesh
{
public:
	D3D12Resource vertices;
	D3D12Resource indices;
	D3D12Heap buffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	uint32_t indexCount() { return indexBufferView.SizeInBytes <= 131070u ? indexBufferView.SizeInBytes / 2u : indexBufferView.SizeInBytes / 4u; }
};