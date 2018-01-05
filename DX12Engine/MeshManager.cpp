#include "MeshManager.h"
#include "SharedResources.h"
#include "BaseExecutor.h"
#include "FixedSizeAllocator.h"

void MeshManager::loadMesh(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, Mesh* mesh),
	void(*useSubresourceCallback)(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset),
	uint32_t vertexStrideInBytes, uint32_t vertexPackedSize, StreamingManagerThreadLocal& streamingManager)
{
	MeshManager& meshManager = executor->sharedResources->meshManager;
	std::unique_lock<decltype(meshManager.mutex)> lock(meshManager.mutex);
	MeshInfo& meshInfo = meshManager.meshInfos[filename];
	meshInfo.numUsers += 1u;
	if (meshInfo.numUsers == 1u)
	{
		meshManager.uploadRequests[filename].push_back(Request{ requester, resourceUploadedCallback});
		lock.unlock();

		meshManager.loadMeshUncached(streamingManager, filename, useSubresourceCallback, vertexStrideInBytes, vertexPackedSize);
	}
	else if (meshInfo.loaded)
	{
		Mesh* mesh = meshInfo.mesh;
		lock.unlock();
		resourceUploadedCallback(requester, executor, mesh);
	}
	else
	{
		meshManager.uploadRequests[filename].push_back(Request{ requester, resourceUploadedCallback });
	}
}

void MeshManager::loadMeshUncached(StreamingManagerThreadLocal& streamingManager, const wchar_t * filename, void(*useSubresourceCallback)(RamToVramUploadRequest* request,
	BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset), uint32_t vertexStrideInBytes,
	uint32_t vertexPackedSize)
{
	RamToVramUploadRequest& uploadRequest = streamingManager.getUploadRequest();
	uploadRequest.resourceUploadedPointer = meshUploaded;
	uploadRequest.useSubresourcePointer = useSubresourceCallback;
	uploadRequest.requester = reinterpret_cast<void*>(const_cast<wchar_t *>(filename));

	uploadRequest.file.open(filename, ScopedFile::accessRight::genericRead, ScopedFile::shareMode::readMode, ScopedFile::creationMode::openExisting, nullptr);
	MeshHeader meshHeader;
	uploadRequest.file.read(meshHeader);

	uint32_t vertexSizeBytes = vertexStrideInBytes * meshHeader.numVertices;
	uint32_t indexSize;
	if (meshHeader.numIndices != 0u) indexSize = sizeof(uint32_t) * meshHeader.numIndices;
	else indexSize = sizeof(uint32_t) * meshHeader.numVertices;

	uploadRequest.width = vertexSizeBytes + indexSize;
	uploadRequest.height = vertexSizeBytes;
	uploadRequest.depth = 1u;
	uploadRequest.format = DXGI_FORMAT_UNKNOWN;
	uploadRequest.arraySize = 1u;
	uploadRequest.dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadRequest.mipLevels = 1u;
	uploadRequest.mostDetailedMip = 0u;
	uploadRequest.currentArrayIndex = 0u;
	uploadRequest.currentMipLevel = 0u;
}

Mesh* MeshManager::createMesh(FixedSizeAllocator<Mesh>& meshAllocator, const wchar_t* filename, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t vertexStrideInBytes,
	uint32_t indexSizeBytes)
{
	Mesh* mesh = meshAllocator.allocate();

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

	new(&mesh->buffer) D3D12Heap(graphicsDevice, heapDesc);
	new(&mesh->vertices) D3D12Resource(graphicsDevice, mesh->buffer, 0u, [vertexSizeBytes]()
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
	}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr);
	new(&mesh->indices) D3D12Resource(graphicsDevice, mesh->buffer, vertexAlignedSize, [indexSizeBytes]()
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
	}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr);


#ifdef _DEBUG
	std::wstring name = L"vertex buffer: ";
	name += filename;
	mesh->vertices->SetName(name.c_str());
	name = L"index buffer: ";
	name += filename;
	mesh->indices->SetName(name.c_str());
#endif // _DEBUG

	mesh->vertexBufferView.BufferLocation = mesh->vertices->GetGPUVirtualAddress();
	mesh->vertexBufferView.StrideInBytes = vertexStrideInBytes;
	mesh->vertexBufferView.SizeInBytes = vertexSizeBytes;

	mesh->indexBufferView.BufferLocation = mesh->indices->GetGPUVirtualAddress();
	mesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mesh->indexBufferView.SizeInBytes = indexSizeBytes;
	mesh->indexCount = indexSizeBytes / 4u;

	return mesh;
}

void MeshManager::unloadMesh(const wchar_t * const filename, BaseExecutor* const executor)
{
	unsigned int oldUserCount;
	Mesh* mesh;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		auto& meshInfo = meshInfos[filename];
		meshInfo.numUsers -= 1u;
		oldUserCount = meshInfo.numUsers;
		if (oldUserCount == 0u)
		{
			mesh = meshInfo.mesh;
			meshInfos.erase(filename);
		}
	}
	
	if (oldUserCount == 0u)
	{
		mesh->~Mesh();
		executor->meshAllocator.deallocate(mesh);
	}
}

static void createIndices(uint32_t* indexUploadBuffer, Mesh* mesh, RamToVramUploadRequest* request, ID3D12Resource* uploadResource,
	uint64_t uploadResourceOffset, uint32_t byteSize, ID3D12GraphicsCommandList* copyCommandList)
{
	const auto end = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(indexUploadBuffer) + byteSize);
	for (uint32_t i = 0u; indexUploadBuffer != end; ++indexUploadBuffer, ++i)
	{
		*indexUploadBuffer = i;
	}
	copyCommandList->CopyBufferRegion(mesh->indices, 0u, uploadResource, uploadResourceOffset, byteSize);
}

void MeshManager::meshWithPositionTextureNormalTangentBitangentUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
	ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator, MeshManager& meshManager,
	ID3D12Device* graphicsDevice)
{
	wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
	auto vertexSizeBytes = request->height;
	auto indexSizeBytes = (uint32_t)(request->width - vertexSizeBytes);
	auto sizeOnFile = vertexSizeBytes;
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPositionTextureNormalTangentBitangent);
	Mesh* mesh = meshManager.createMesh(meshAllocator, filename, graphicsDevice, vertexSizeBytes, vertexStrideInBytes,
		indexSizeBytes);
	
	{
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		auto& meshInfo = meshManager.meshInfos[filename];
		meshInfo.mesh = mesh;
	}

	ID3D12GraphicsCommandList* copyCommandList = streamingManager.currentCommandList;
	MeshWithPositionTextureNormalTangentBitangent* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTextureNormalTangentBitangent* const>(uploadBufferCpuAddressOfCurrentPos);

	{
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[sizeOnFile]);
		request->file.read(buffer.get(), sizeOnFile);
		request->file.close();
		CalculateTangentBitangent(buffer.get(), buffer.get() + sizeOnFile, vertexUploadBuffer);
	}
	
	copyCommandList->CopyBufferRegion(mesh->vertices, 0u, uploadResource, uploadResourceOffset, vertexSizeBytes);

	createIndices(reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(uploadBufferCpuAddressOfCurrentPos) + vertexSizeBytes), mesh, request, uploadResource,
		uploadResourceOffset + vertexSizeBytes, indexSizeBytes, copyCommandList);
}

void MeshManager::meshWithPositionTextureNormalUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
	ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator, MeshManager& meshManager,
	ID3D12Device* graphicsDevice)
{
	wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
	auto vertexSizeBytes = request->height;
	auto indexSizeBytes = (uint32_t)(request->width - vertexSizeBytes);
	auto sizeOnFile = vertexSizeBytes;
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPositionTextureNormal);
	Mesh* mesh = meshManager.createMesh(meshAllocator, filename, graphicsDevice, vertexSizeBytes, vertexStrideInBytes,
		indexSizeBytes);

	{
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		auto& meshInfo = meshManager.meshInfos[filename];
		meshInfo.mesh = mesh;
	}

	ID3D12GraphicsCommandList* copyCommandList = streamingManager.currentCommandList;
	MeshWithPositionTextureNormal* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTextureNormal* const>(uploadBufferCpuAddressOfCurrentPos);

	{
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[sizeOnFile]);
		request->file.read(buffer.get(), sizeOnFile);
		request->file.close();
		auto end = buffer.get() + sizeOnFile;
		auto start = buffer.get();
		while (start != end)
		{
			vertexUploadBuffer->x = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->y = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->z = *reinterpret_cast<float*>(start);
			start += sizeof(float);

			vertexUploadBuffer->tu = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->tv = *reinterpret_cast<float*>(start);
			start += sizeof(float);

			vertexUploadBuffer->nx = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->ny = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->nz = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			++vertexUploadBuffer;
		}
	}
	copyCommandList->CopyBufferRegion(mesh->vertices, 0u, uploadResource, uploadResourceOffset, vertexSizeBytes);

	createIndices(reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(uploadBufferCpuAddressOfCurrentPos) + vertexSizeBytes), mesh, request, uploadResource,
		uploadResourceOffset + vertexSizeBytes, indexSizeBytes, copyCommandList);
}

void MeshManager::meshWithPositionTextureUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
	ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator, MeshManager& meshManager,
	ID3D12Device* graphicsDevice)
{
	wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
	auto vertexSizeBytes = request->height;
	auto indexSizeBytes = (uint32_t)(request->width - vertexSizeBytes);
	auto sizeOnFile = vertexSizeBytes;
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPositionTexture);
	Mesh* mesh = meshManager.createMesh(meshAllocator, filename, graphicsDevice, vertexSizeBytes, vertexStrideInBytes, indexSizeBytes);

	{
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		auto& meshInfo = meshManager.meshInfos[filename];
		meshInfo.mesh = mesh;
	}

	ID3D12GraphicsCommandList* copyCommandList = streamingManager.currentCommandList;
	MeshWithPositionTexture* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTexture* const>(uploadBufferCpuAddressOfCurrentPos);

	{
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[sizeOnFile]);
		request->file.read(buffer.get(), static_cast<uint32_t>(sizeOnFile));
		request->file.close();
		auto end = buffer.get() + sizeOnFile;
		auto start = buffer.get();
		while (start != end)
		{
			vertexUploadBuffer->x = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->y = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->z = *reinterpret_cast<float*>(start);
			start += sizeof(float);

			vertexUploadBuffer->tu = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->tv = *reinterpret_cast<float*>(start);
			start += sizeof(float);

			++vertexUploadBuffer;
		}
	}
	copyCommandList->CopyBufferRegion(mesh->vertices, 0u, uploadResource, uploadResourceOffset, vertexSizeBytes);

	createIndices(reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(uploadBufferCpuAddressOfCurrentPos) + vertexSizeBytes), mesh, request, uploadResource,
		uploadResourceOffset + vertexSizeBytes, indexSizeBytes, copyCommandList);
}

void MeshManager::meshWithPositionUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
	ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator,
	MeshManager& meshManager, ID3D12Device* graphicsDevice)
{
	wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
	auto vertexSizeBytes = request->height;
	auto indexSizeBytes = (uint32_t)(request->width - vertexSizeBytes);
	auto sizeOnFile = vertexSizeBytes;
	constexpr auto vertexStrideInBytes = sizeof(MeshWithPosition);
	Mesh* mesh = meshManager.createMesh(meshAllocator, filename, graphicsDevice, vertexSizeBytes, vertexStrideInBytes, indexSizeBytes);

	{
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		auto& meshInfo = meshManager.meshInfos[filename];
		meshInfo.mesh = mesh;
	}

	ID3D12GraphicsCommandList* copyCommandList = streamingManager.currentCommandList;
	MeshWithPosition* vertexUploadBuffer = reinterpret_cast<MeshWithPosition* const>(uploadBufferCpuAddressOfCurrentPos);

	{
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[sizeOnFile]);
		request->file.read(uploadBufferCpuAddressOfCurrentPos, sizeOnFile);
		request->file.close();
	}
	copyCommandList->CopyBufferRegion(mesh->vertices, 0u, uploadResource, uploadResourceOffset, vertexSizeBytes);
	;
	createIndices(reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(uploadBufferCpuAddressOfCurrentPos) + vertexSizeBytes), mesh, request, uploadResource,
		uploadResourceOffset + vertexSizeBytes, indexSizeBytes, copyCommandList);
}

void MeshManager::meshUploaded(void* requester, BaseExecutor* executor)
{
	MeshManager& meshManager = executor->sharedResources->meshManager;
	const wchar_t* filename = reinterpret_cast<wchar_t*>(requester);
	Mesh* mesh;
	std::vector<Request> requests;
	{
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		MeshInfo* meshInfo = &meshManager.meshInfos[filename];
		meshInfo->loaded = true;

		mesh = meshInfo->mesh;

		auto request = meshManager.uploadRequests.find(filename);
		if (request != meshManager.uploadRequests.end())
		{
			requests = std::move(request->second);
			meshManager.uploadRequests.erase(request);
		}
	}

	for (auto& request : requests)
	{
		request.resourceUploaded(request.requester, executor, mesh);
	}
}

void MeshManager::CalculateTangentBitangent(unsigned char* start, unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh)
{
	float vector1[3], vector2[3];
	float tuVector[2], tvVector[2];
	float OneOverlength;
	MeshWithPositionTextureNormalTangentBitangent meshes[3];
	
	while (start != end)
	{
		for (auto i = 0u; i < 3; ++i)
		{
			meshes[i].x = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			meshes[i].y = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			meshes[i].z = *reinterpret_cast<float*>(start);
			start += sizeof(float);

			meshes[i].tu = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			meshes[i].tv = *reinterpret_cast<float*>(start);
			start += sizeof(float);

			meshes[i].nx = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			meshes[i].ny = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			meshes[i].nz = *reinterpret_cast<float*>(start);
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
		tuVector[0] = meshes[1].tv - meshes[0].tv;

		tuVector[1] = meshes[2].tu - meshes[0].tu;
		tuVector[1] = meshes[2].tv - meshes[0].tv;

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

MeshManager::MeshManager() {}
MeshManager::~MeshManager() {}