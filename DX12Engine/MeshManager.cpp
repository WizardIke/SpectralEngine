#include "MeshManager.h"
#include "SharedResources.h"
#include "BaseExecutor.h"

void MeshManager::loadMesh(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor),
	void(*useSubresourceCallback)(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset), uint32_t vertexStrideInBytes, Mesh** destination,
	uint32_t vertexPackedSize)
{
	MeshManager& meshManager = executor->sharedResources->meshManager;
	std::unique_lock<decltype(meshManager.mutex)> lock(meshManager.mutex);
	MeshInfo& meshInfo = meshManager.meshInfos[filename];
	meshInfo.numUsers += 1u;
	if (meshInfo.numUsers == 1u)
	{
		meshManager.uploadRequests[filename].push_back(PendingLoadRequest{ requester, resourceUploadedCallback,  destination });
		lock.unlock();

		meshManager.loadMeshUncached(executor, filename, useSubresourceCallback, vertexStrideInBytes, vertexPackedSize);
	}
	else if (meshInfo.loaded)
	{
		Mesh* mesh = meshInfo.mesh;
		lock.unlock();
		*destination = mesh;
		resourceUploadedCallback(requester, executor);
	}
	else
	{
		meshManager.uploadRequests[filename].push_back(PendingLoadRequest{ requester, resourceUploadedCallback, destination });
	}
}

void MeshManager::loadMeshUncached(BaseExecutor* const executor, const wchar_t * filename, void(*useSubresourceCallback)(RamToVramUploadRequest* request,
	BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset), uint32_t vertexStrideInBytes, uint32_t vertexPackedSize)
{
	RamToVramUploadRequest& uploadRequest = executor->streamingManager.getUploadRequest();
	uploadRequest.resourceUploadedPointer = meshUploaded;
	uploadRequest.useSubresourcePointer = useSubresourceCallback;
	uploadRequest.requester = reinterpret_cast<void*>(const_cast<wchar_t *>(filename));

	uploadRequest.file.open(filename, ScopedFile::accessRight::genericRead, ScopedFile::shareMode::readMode, ScopedFile::creationMode::openExisting, nullptr);

	MeshHeader meshHeader;
	uploadRequest.file.read(meshHeader);

	uint32_t vertexSize = vertexStrideInBytes * meshHeader.numVertices;
	uint64_t vertexAlignedSize = (vertexSize + (uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u) & ~((uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u);
	uint32_t indexSize;
	if (meshHeader.numIndices != 0u) indexSize = sizeof(uint32_t) * meshHeader.numIndices;
	else indexSize = sizeof(uint32_t) * meshHeader.numVertices;
	uint64_t indexAlignedSize = (indexSize + (uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u) & ~((uint64_t)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - (uint64_t)1u);

	uploadRequest.width = vertexPackedSize * meshHeader.numVertices + indexSize;
	uploadRequest.height = 1u;
	uploadRequest.depth = 1u;
	uploadRequest.format = DXGI_FORMAT_UNKNOWN;
	uploadRequest.uploadSizeInBytes = uploadRequest.width;
	uploadRequest.numSubresources = 1u;

	uint64_t heapSizeInBytes = vertexAlignedSize + indexAlignedSize;
	D3D12_HEAP_DESC heapDesc;
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDesc.Properties.CreationNodeMask = 1u;
	heapDesc.Properties.VisibleNodeMask = 1u;
	heapDesc.SizeInBytes = heapSizeInBytes;

	ID3D12Device* graphicsDevice = executor->sharedResources->graphicsEngine.graphicsDevice;

	Mesh* mesh = executor->meshAllocator.allocate();
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		auto& meshInfo = meshInfos[filename];
		meshInfo.mesh = mesh;
	}
	

	new(&mesh->buffer) D3D12Heap(graphicsDevice, heapDesc);
	new(&mesh->vertices) D3D12Resource(graphicsDevice, mesh->buffer, 0u, [vertexSize]()
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
		resourceDesc.Width = vertexSize;
		return resourceDesc;
	}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr);
	new(&mesh->indices) D3D12Resource(graphicsDevice, mesh->buffer, vertexAlignedSize, [indexSize]()
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
		resourceDesc.Width = indexSize;
		return resourceDesc;
	}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr);


#ifdef _DEBUG
	wchar_t name[100] = L"vertex buffer: ";
	wcscat_s(name, filename);
	wchar_t name2[100] = L"index buffer: ";
	wcscat_s(name2, filename);

	mesh->vertices->SetName(name);
	mesh->indices->SetName(name2);
#endif // _DEBUG

	mesh->vertexBufferView.BufferLocation = mesh->vertices->GetGPUVirtualAddress();
	mesh->vertexBufferView.StrideInBytes = vertexStrideInBytes;
	mesh->vertexBufferView.SizeInBytes = vertexSize;
	mesh->vertexCount = meshHeader.numVertices;

	mesh->indexBufferView.BufferLocation = mesh->indices->GetGPUVirtualAddress();
	mesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mesh->indexBufferView.SizeInBytes = indexSize;
	mesh->indexCount = meshHeader.numIndices;
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
		executor->meshAllocator.deallocate(mesh);
	}
}

static void loadIndices(uint32_t* indexUploadBuffer, Mesh* mesh, RamToVramUploadRequest* request, BaseExecutor* executor, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, uint32_t byteSize)
{
	if (!mesh->indexCount)
	{
		const auto vertexCount = mesh->vertexCount;
		mesh->indexCount = vertexCount;
		for (uint32_t j = 0u; j < vertexCount; ++j, ++indexUploadBuffer)
		{
			*indexUploadBuffer = j;
		}
	}
	else
	{
		request->file.read(indexUploadBuffer, mesh->indexCount);
		request->file.close();
	}

	executor->streamingManager.currentCommandList->CopyBufferRegion(mesh->indices, 0u, uploadResource, uploadResourceOffset, byteSize);
}

void MeshManager::meshWithPositionTextureNormalTangentBitangentUseSubresource(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
	uint64_t uploadResourceOffset)
{
	MeshWithPositionTextureNormalTangentBitangent* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTextureNormalTangentBitangent* const>(uploadBufferCpuAddressOfCurrentPos);

	MeshManager& meshManager = executor->sharedResources->meshManager;
	Mesh* mesh;
	{
		wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		mesh = meshManager.meshInfos.find(filename)->second.mesh;
	}

	const auto vertexCount = mesh->vertexCount;
	uint32_t byteSize = vertexCount * sizeof(float) * 8u;
	{
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[byteSize]);
		request->file.read(buffer.get(), static_cast<uint32_t>(byteSize));
		request->file.close();
		CalculateTangentBitangent(buffer.get(), buffer.get() + byteSize, vertexUploadBuffer);
	}
	
	uint32_t vertexSize = vertexCount * sizeof(MeshWithPositionTextureNormalTangentBitangent);
	executor->streamingManager.currentCommandList->CopyBufferRegion(mesh->vertices, 0u, uploadResource, uploadResourceOffset, vertexSize);

	const uint32_t indexSize = request->width - byteSize;
	loadIndices(reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(uploadBufferCpuAddressOfCurrentPos) + byteSize), mesh, request, executor, uploadResource, uploadResourceOffset + byteSize, indexSize);
}

void MeshManager::meshWithPositionTextureNormalUseSubresource(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
	uint64_t uploadResourceOffset)
{
	MeshWithPositionTextureNormal* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTextureNormal* const>(uploadBufferCpuAddressOfCurrentPos);

	MeshManager& meshManager = executor->sharedResources->meshManager;
	Mesh* mesh;
	{
		wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		mesh = meshManager.meshInfos.find(filename)->second.mesh;
	}

	const auto vertexCount = mesh->vertexCount;
	uint32_t byteSize = vertexCount * sizeof(float) * 8u;
	{
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[byteSize]);
		request->file.read(buffer.get(), static_cast<uint32_t>(byteSize));
		request->file.close();
		auto end = buffer.get() + byteSize;
		auto start = buffer.get();
		while (start != end)
		{
			vertexUploadBuffer->x = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->y = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->z = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->w = 1.0f;

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
	uint32_t vertexSize = vertexCount * sizeof(MeshWithPositionTextureNormal);
	executor->streamingManager.currentCommandList->CopyBufferRegion(mesh->vertices, 0u, uploadResource, uploadResourceOffset, vertexSize);

	const uint32_t indexSize = request->width - byteSize;
	loadIndices(reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(uploadBufferCpuAddressOfCurrentPos) + byteSize), mesh, request, executor, uploadResource, uploadResourceOffset + byteSize, indexSize);
}

void MeshManager::meshWithPositionTextureUseSubresource(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
	uint64_t uploadResourceOffset)
{
	MeshWithPositionTexture* vertexUploadBuffer = reinterpret_cast<MeshWithPositionTexture* const>(uploadBufferCpuAddressOfCurrentPos);

	MeshManager& meshManager = executor->sharedResources->meshManager;
	Mesh* mesh;
	{
		wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		mesh = meshManager.meshInfos.find(filename)->second.mesh;
	}

	const auto vertexCount = mesh->vertexCount;
	uint32_t byteSize = vertexCount * sizeof(float) * 5u;
	{
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[byteSize]);
		request->file.read(buffer.get(), static_cast<uint32_t>(byteSize));
		request->file.close();
		auto end = buffer.get() + byteSize;
		auto start = buffer.get();
		while (start != end)
		{
			vertexUploadBuffer->x = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->y = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->z = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->w = 1.0f;

			vertexUploadBuffer->tu = *reinterpret_cast<float*>(start);
			start += sizeof(float);
			vertexUploadBuffer->tv = *reinterpret_cast<float*>(start);
			start += sizeof(float);

			++vertexUploadBuffer;
		}
	}
	uint32_t vertexSize = vertexCount * sizeof(MeshWithPositionTexture);
	executor->streamingManager.currentCommandList->CopyBufferRegion(mesh->vertices, 0u, uploadResource, uploadResourceOffset, vertexSize);

	const uint32_t indexSize = request->width - byteSize;
	loadIndices(reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(uploadBufferCpuAddressOfCurrentPos) + byteSize), mesh, request, executor, uploadResource, uploadResourceOffset + byteSize, indexSize);
}

void MeshManager::meshUploaded(void* requester, BaseExecutor* executor)
{
	MeshManager& meshManager = executor->sharedResources->meshManager;
	wchar_t* filename = reinterpret_cast<wchar_t*>(requester);
	MeshInfo* meshInfo;
	Mesh* mesh;
	std::vector<PendingLoadRequest> requests;
	{
		std::lock_guard<decltype(meshManager.mutex)> lock(meshManager.mutex);
		meshInfo = &meshManager.meshInfos[filename];
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
		*(request.destination) = mesh;
		request.resourceUploaded(request.requester, executor);
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
		Mesh->w = 1.f;
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
		Mesh->w = 1.f;
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
		Mesh->w = 1.f;
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