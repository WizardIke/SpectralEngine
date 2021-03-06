#include "MeshManager.h"
#include <cassert>
#include <cstring> //std::memcpy

MeshManager::MeshManager(AsynchronousFileManager& asynchronousFileManager, StreamingManager& streamingManager, ID3D12Device& graphicsDevice) :
	asynchronousFileManager(asynchronousFileManager),
	streamingManager(streamingManager),
	graphicsDevice(graphicsDevice)
{}

void MeshManager::fillUploadRequestHelper(MeshStreamingRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t vertexStride)
{
	uint32_t indicesSize;
	if (indexCount != 0u)
	{
		if (indexCount <= 65535u)
		{
			indicesSize = sizeof(uint16_t) * indexCount;
		}
		else
		{
			indicesSize = sizeof(uint32_t) * indexCount;
		}
	}
	else
	{
		if (vertexCount <= 65535u)
		{
			indicesSize = sizeof(uint16_t) * vertexCount;
		}
		else
		{
			indicesSize = sizeof(uint32_t) * vertexCount;
		}
	}

	uploadRequest.verticesSize = vertexCount * vertexStride;
	uploadRequest.indicesSize = indicesSize;
	uploadRequest.resourceSize = uploadRequest.verticesSize + indicesSize;
	uploadRequest.sizeOnFile = indexCount != 0u ? uploadRequest.resourceSize : uploadRequest.verticesSize;
	uploadRequest.vertexStride = vertexStride;
}

void MeshManager::createMeshResources(ID3D12Resource*& vertices, ID3D12Resource*& indices, ID3D12Heap*& meshBuffer, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t indexSizeBytes)
{
	uint64_t vertexAlignedSize = (vertexSizeBytes + (uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u) &
		~((uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u);
	uint64_t indexAlignedSize = (indexSizeBytes + (uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u) &
		~((uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u);

	D3D12_HEAP_DESC heapDesc;
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDesc.Properties.CreationNodeMask = 1u;
	heapDesc.Properties.VisibleNodeMask = 1u;
	heapDesc.SizeInBytes = vertexAlignedSize + indexAlignedSize;

	meshBuffer = D3D12Heap(graphicsDevice, heapDesc).steal();

	vertices = D3D12Resource(graphicsDevice, meshBuffer, 0u, [vertexSizeBytes]()
	{
		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.DepthOrArraySize = 1u;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Height = 1u;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.MipLevels = 1u;
		resourceDesc.SampleDesc.Count = 1u;
		resourceDesc.SampleDesc.Quality = 0u;
		resourceDesc.Width = vertexSizeBytes;
		return resourceDesc;
	}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON).steal();

	indices = D3D12Resource(graphicsDevice, meshBuffer, vertexAlignedSize, [indexSizeBytes]()
	{
		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.DepthOrArraySize = 1u;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Height = 1u;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.MipLevels = 1u;
		resourceDesc.SampleDesc.Count = 1u;
		resourceDesc.SampleDesc.Quality = 0u;
		resourceDesc.Width = indexSizeBytes;
		return resourceDesc;
	}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON).steal();
}

void MeshManager::unloadMesh(UnloadRequest& unloadRequest, void* tr)
{
	auto meshPtr = meshInfos.find({ unloadRequest.start, unloadRequest.end });
	auto& meshInfo = meshPtr->second;
	--meshInfo.numUsers;
	if(meshInfo.numUsers == 0u)
	{
		meshInfos.erase(meshPtr);
	}

	unloadRequest.deleteReadRequest(unloadRequest, tr);
}

static void createIndices(void* indexUploadBuffer, ID3D12Resource* indices, ID3D12Resource* uploadResource,
	uint64_t uploadResourceOffset, uint32_t byteSize, ID3D12GraphicsCommandList* copyCommandList)
{
	if (byteSize <= 131070u)
	{
		uint16_t* typedIndexUploadBuffer = static_cast<uint16_t*>(indexUploadBuffer);
		const auto end = reinterpret_cast<uint16_t*>(static_cast<unsigned char*>(indexUploadBuffer) + byteSize);
		for (uint16_t i = 0u; typedIndexUploadBuffer != end; ++typedIndexUploadBuffer, ++i)
		{
			*typedIndexUploadBuffer = i;
		}
	}
	else
	{
		uint32_t* typedIndexUploadBuffer = static_cast<uint32_t*>(indexUploadBuffer);
		const auto end = reinterpret_cast<uint32_t*>(static_cast<unsigned char*>(indexUploadBuffer) + byteSize);
		for (uint32_t i = 0u; typedIndexUploadBuffer != end; ++typedIndexUploadBuffer, ++i)
		{
			*typedIndexUploadBuffer = i;
		}
	}
	copyCommandList->CopyBufferRegion(indices, 0u, uploadResource, uploadResourceOffset, byteSize);
}

void MeshManager::meshWithPositionTextureNormalTangentBitangentUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
	StreamingManager::ThreadLocal& streamingManager, void(*copyStarted)(void* requester, void* tr))
{
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPositionTextureNormalTangentBitangent);
	auto vertexSizeBytes = uploadRequest.verticesSize;
	auto vertexSizeBytesInFile = vertexSizeBytes / vertexStrideInBytes * sizeof(MeshWithPositionTextureNormal);
	auto indexSizeBytes = uploadRequest.indicesSize;

	createMeshResources(uploadRequest.vertices, uploadRequest.indices, uploadRequest.meshBuffer, graphicsDevice, vertexSizeBytes, indexSizeBytes);

	ID3D12GraphicsCommandList& copyCommandList = streamingManager.copyCommandList();
	MeshWithPositionTextureNormalTangentBitangent* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTextureNormalTangentBitangent* const>(uploadRequest.uploadBufferCurrentCpuAddress);

	CalculateTangentBitangent(buffer, buffer + vertexSizeBytesInFile, vertexUploadBuffer);

	copyCommandList.CopyBufferRegion(uploadRequest.vertices, 0u, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, vertexSizeBytes);

	createIndices(uploadRequest.uploadBufferCurrentCpuAddress + vertexSizeBytes, uploadRequest.indices, uploadRequest.uploadResource,
		uploadRequest.uploadResourceOffset + vertexSizeBytes, indexSizeBytes, &copyCommandList);

	streamingManager.addCopyCompletionEvent(&uploadRequest, copyStarted);
}

void MeshManager::meshNoConvertUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
	StreamingManager::ThreadLocal& streamingManager, void(*copyStarted)(void* requester, void* tr))
{
	auto vertexSizeBytes = uploadRequest.verticesSize;
	auto indexSizeBytes = uploadRequest.indicesSize;
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPositionTextureNormal);

	createMeshResources(uploadRequest.vertices, uploadRequest.indices, uploadRequest.meshBuffer, graphicsDevice, vertexSizeBytes, indexSizeBytes);

	std::memcpy(uploadRequest.uploadBufferCurrentCpuAddress, buffer, vertexSizeBytes);
	auto& copyCommandList = streamingManager.copyCommandList();
	copyCommandList.CopyBufferRegion(uploadRequest.vertices, 0u, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, vertexSizeBytes);

	createIndices(uploadRequest.uploadBufferCurrentCpuAddress + vertexSizeBytes, uploadRequest.indices, uploadRequest.uploadResource,
		uploadRequest.uploadResourceOffset + vertexSizeBytes, indexSizeBytes, &copyCommandList);

	streamingManager.addCopyCompletionEvent(&uploadRequest, copyStarted);
}

void MeshManager::meshWithPositionTextureNormalTangentBitangentIndexUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
	StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr))
{
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPositionTextureNormalTangentBitangent);
	auto vertexSizeBytes = uploadRequest.verticesSize;
	auto vertexSizeBytesInFile = vertexSizeBytes / vertexStrideInBytes * sizeof(MeshWithPositionTextureNormal);
	auto indexSizeBytes = uploadRequest.indicesSize;

	createMeshResources(uploadRequest.vertices, uploadRequest.indices, uploadRequest.meshBuffer, graphicsDevice, vertexSizeBytes, indexSizeBytes);

	ID3D12GraphicsCommandList& copyCommandList = streamingManager.copyCommandList();
	MeshWithPositionTextureNormalTangentBitangent* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTextureNormalTangentBitangent* const>(uploadRequest.uploadBufferCurrentCpuAddress);
	CalculateTangentBitangent(buffer, buffer + vertexSizeBytesInFile, vertexUploadBuffer);
	copyCommandList.CopyBufferRegion(uploadRequest.vertices, 0u, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, vertexSizeBytes);

	std::memcpy(uploadRequest.uploadBufferCurrentCpuAddress + vertexSizeBytes, buffer + vertexSizeBytesInFile, indexSizeBytes);
	copyCommandList.CopyBufferRegion(uploadRequest.indices, 0u, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset + vertexSizeBytes, indexSizeBytes);

	streamingManager.addCopyCompletionEvent(&uploadRequest, copyFinished);
}

void MeshManager::meshNoConvertIndexUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
	StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr))
{
	auto vertexSizeBytes = uploadRequest.verticesSize;
	auto indexSizeBytes = uploadRequest.indicesSize;
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPositionTextureNormal);

	createMeshResources(uploadRequest.vertices, uploadRequest.indices, uploadRequest.meshBuffer, graphicsDevice, vertexSizeBytes, indexSizeBytes);

	std::memcpy(uploadRequest.uploadBufferCurrentCpuAddress, buffer, vertexSizeBytes);
	auto& copyCommandList = streamingManager.copyCommandList();
	copyCommandList.CopyBufferRegion(uploadRequest.vertices, 0u, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, vertexSizeBytes);

	std::memcpy(uploadRequest.uploadBufferCurrentCpuAddress + vertexSizeBytes, buffer + vertexSizeBytes, indexSizeBytes);
	copyCommandList.CopyBufferRegion(uploadRequest.indices, 0u, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset + vertexSizeBytes, indexSizeBytes);

	streamingManager.addCopyCompletionEvent(&uploadRequest, copyFinished);
}

void MeshManager::notifyMeshReady(MeshStreamingRequest* request, void* tr)
{
	MeshInfo& meshInfo = meshInfos[request->resourceLocation];
	meshInfo.lastRequest = nullptr;
	Mesh& mesh = meshInfo.mesh;
	fillMesh(mesh, *request);
	do
	{
		auto old = request;
		request = request->nextMeshRequest; //Need to do this now as old could be reused by the next line
		old->meshLoaded(*old, tr, mesh);
	} while(request != nullptr);
}

void MeshManager::fillMesh(Mesh& mesh, MeshStreamingRequest& request)
{
	mesh.buffer.set() = request.meshBuffer;
	mesh.indices.set() = request.indices;
	mesh.vertices.set() = request.vertices;

	mesh.vertexBufferView.BufferLocation = mesh.vertices->GetGPUVirtualAddress();
	mesh.vertexBufferView.StrideInBytes = request.vertexStride;
	mesh.vertexBufferView.SizeInBytes = request.verticesSize;

	mesh.indexBufferView.BufferLocation = mesh.indices->GetGPUVirtualAddress();
	mesh.indexBufferView.Format = request.indicesSize <= 131070u ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	mesh.indexBufferView.SizeInBytes = request.indicesSize;
}

void MeshManager::CalculateTangentBitangent(const unsigned char* start, const unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh)
{
	float vector1[3], vector2[3];
	float tuVector[2], tvVector[2];
	float OneOverlength;
	MeshWithPositionTextureNormalTangentBitangent meshes[3];
	
	while (start != end)
	{
		for (auto i = 0u; i < 3; ++i)
		{
			meshes[i].x = *reinterpret_cast<const float*>(start);
			start += sizeof(float);
			meshes[i].y = *reinterpret_cast<const float*>(start);
			start += sizeof(float);
			meshes[i].z = *reinterpret_cast<const float*>(start);
			start += sizeof(float);

			meshes[i].tu = *reinterpret_cast<const float*>(start);
			start += sizeof(float);
			meshes[i].tv = *reinterpret_cast<const float*>(start);
			start += sizeof(float);

			meshes[i].nx = *reinterpret_cast<const float*>(start);
			start += sizeof(float);
			meshes[i].ny = *reinterpret_cast<const float*>(start);
			start += sizeof(float);
			meshes[i].nz = *reinterpret_cast<const float*>(start);
			start += sizeof(float);
		}

		// Calculate the two vectors for this face.
		vector1[0] = meshes[1].x - meshes[0].x;
		vector1[1] = meshes[1].y - meshes[0].y;
		vector1[2] = meshes[1].z - meshes[0].z;

		vector2[0] = meshes[2].x - meshes[0].x;
		vector2[1] = meshes[2].y - meshes[0].y;
		vector2[2] = meshes[2].z - meshes[0].z;

		tuVector[0] = meshes[1].tu - meshes[0].tu;
		tvVector[0] = meshes[1].tv - meshes[0].tv;

		tuVector[1] = meshes[2].tu - meshes[0].tu;
		tvVector[1] = meshes[2].tv - meshes[0].tv;

		meshes[0].tx = (tvVector[1] * vector1[0] - tvVector[0] * vector2[0]);
		meshes[0].ty = (tvVector[1] * vector1[1] - tvVector[0] * vector2[1]);
		meshes[0].tz = (tvVector[1] * vector1[2] - tvVector[0] * vector2[2]);

		meshes[0].bx = (tuVector[0] * vector2[0] - tuVector[1] * vector1[0]);
		meshes[0].by = (tuVector[0] * vector2[1] - tuVector[1] * vector1[1]);
		meshes[0].bz = (tuVector[0] * vector2[2] - tuVector[1] * vector1[2]);

		// Calculate the length of this normal.
		OneOverlength = 1 / sqrt((meshes[0].tx * meshes[0].tx) + (meshes[0].ty * meshes[0].ty) + (meshes[0].tz * meshes[0].tz));

		// Normalize the normal and then store it
		meshes[0].tx *= OneOverlength;
		meshes[0].ty *= OneOverlength;
		meshes[0].tz *= OneOverlength;

		// Calculate the length of this normal.
		OneOverlength = 1 / sqrt((meshes[0].bx * meshes[0].bx) + (meshes[0].by * meshes[0].by) + (meshes[0].bz * meshes[0].bz));

		// Normalize the normal and then store it
		meshes[0].bx *= OneOverlength;
		meshes[0].by *= OneOverlength;
		meshes[0].bz *= OneOverlength;

		Mesh->x = meshes[0].x;
		Mesh->y = meshes[0].y;
		Mesh->z = meshes[0].z;
		Mesh->tu = meshes[0].tu;
		Mesh->tv = meshes[0].tv;
		Mesh->nx = meshes[0].nx;
		Mesh->ny = meshes[0].ny;
		Mesh->nz = meshes[0].nz;
		Mesh->tx = meshes[0].tx;
		Mesh->ty = meshes[0].ty;
		Mesh->tz = meshes[0].tz;
		Mesh->bx = meshes[0].bx;
		Mesh->by = meshes[0].by;
		Mesh->bz = meshes[0].bz;
		++Mesh;

		Mesh->x = meshes[1].x;
		Mesh->y = meshes[1].y;
		Mesh->z = meshes[1].z;
		Mesh->tu = meshes[1].tu;
		Mesh->tv = meshes[1].tv;
		Mesh->nx = meshes[1].nx;
		Mesh->ny = meshes[1].ny;
		Mesh->nz = meshes[1].nz;
		Mesh->tx = meshes[0].tx;
		Mesh->ty = meshes[0].ty;
		Mesh->tz = meshes[0].tz;
		Mesh->bx = meshes[0].bx;
		Mesh->by = meshes[0].by;
		Mesh->bz = meshes[0].bz;
		++Mesh;

		Mesh->x = meshes[2].x;
		Mesh->y = meshes[2].y;
		Mesh->z = meshes[2].z;
		Mesh->tu = meshes[2].tu;
		Mesh->tv = meshes[2].tv;
		Mesh->nx = meshes[2].nx;
		Mesh->ny = meshes[2].ny;
		Mesh->nz = meshes[2].nz;
		Mesh->tx = meshes[0].tx;
		Mesh->ty = meshes[0].ty;
		Mesh->tz = meshes[0].tz;
		Mesh->bx = meshes[0].bx;
		Mesh->by = meshes[0].by;
		Mesh->bz = meshes[0].bz;
		++Mesh;
	}
}

uint32_t MeshManager::getVertexStride(uint32_t vertexType) noexcept
{
	assert(vertexType < 6u);
	static constexpr uint32_t vertexStrides[6] = { 12u, 20u, 32u, 52u, 24u, 28u };
	return vertexStrides[vertexType];
}