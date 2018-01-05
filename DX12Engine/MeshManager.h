#pragma once
#include "Mesh.h"
#include "RamToVramUploadRequest.h"
#include <vector>
#include <unordered_map>
#include <mutex>
class BaseExecutor;
class StreamingManagerThreadLocal;
template<class T>
class FixedSizeAllocator;

class MeshManager
{
	struct MeshWithPositionTextureNormalTangentBitangent
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
		float tx, ty, tz;
		float bx, by, bz;
	};

	struct MeshWithPositionTextureNormal
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	struct MeshWithPositionTexture
	{
		float x, y, z;
		float tu, tv;
	};

	struct MeshWithPosition
	{
		float x, y, z;
	};

	struct Request
	{
		void* requester;
		void(*resourceUploaded)(void* const requester, BaseExecutor* const executor, Mesh* mesh);
	};

	struct MeshInfo
	{
		MeshInfo() : numUsers(0u), loaded(false) {}
		Mesh* mesh;
		unsigned int numUsers;
		bool loaded;
	};

	struct MeshHeader
	{
		uint32_t vertexType;
		uint32_t numVertices;
		uint32_t numIndices;
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, std::vector<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, MeshInfo, std::hash<const wchar_t *>> meshInfos;

	static void loadMesh(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, Mesh* mesh),
		void(*useSubresourceCallback)(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset),
		uint32_t vertexStrideInBytes, uint32_t vertexPackedSize, StreamingManagerThreadLocal& streamingManager);

	void loadMeshUncached(StreamingManagerThreadLocal& streamingManager, const wchar_t * filename, void(*useSubresourceCallback)(RamToVramUploadRequest* request, BaseExecutor* executor,
		void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset), uint32_t vertexStrideInBytes, uint32_t vertexPackedSize);

	static void meshUploaded(void* storedFilename, BaseExecutor* executor);

	template<class Executor>
	static void meshWithPositionTextureNormalTangentBitangentUseSubresource(RamToVramUploadRequest* request, BaseExecutor* exe,
		void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
	{
		const auto executor = reinterpret_cast<Executor*>(exe);
		meshWithPositionTextureNormalTangentBitangentUseSubresourceHelper(request, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset, 
			executor->streamingManager, executor->meshAllocator, executor->sharedResources->meshManager, executor->sharedResources->graphicsEngine.graphicsDevice);
	}

	static void MeshManager::meshWithPositionTextureNormalTangentBitangentUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
		ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator, MeshManager& meshManager,
		ID3D12Device* graphicsDevice);

	template<class Executor>
	static void meshWithPositionTextureNormalUseSubresource(RamToVramUploadRequest* request, BaseExecutor* exe,
		void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
	{
		const auto executor = reinterpret_cast<Executor*>(exe);
		meshWithPositionTextureNormalUseSubresourceHelper(request, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset,
			executor->streamingManager, executor->meshAllocator, executor->sharedResources->meshManager, executor->sharedResources->graphicsEngine.graphicsDevice);
	}

	static void meshWithPositionTextureNormalUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
		ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator, MeshManager& meshManager,
		ID3D12Device* graphicsDevice);

	template<class Executor>
	static void meshWithPositionTextureUseSubresource(RamToVramUploadRequest* request, BaseExecutor* exe,
		void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
	{
		const auto executor = reinterpret_cast<Executor*>(exe);
		meshWithPositionTextureUseSubresourceHelper(request, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset,
			executor->streamingManager, executor->meshAllocator, executor->sharedResources->meshManager, executor->sharedResources->graphicsEngine.graphicsDevice);
	}

	static void meshWithPositionTextureUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
		ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator, MeshManager& meshManager,
		ID3D12Device* graphicsDevice);

	template<class Executor>
	static void meshWithPositionUseSubresource(RamToVramUploadRequest* request, BaseExecutor* exe,
		void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
	{
		const auto executor = reinterpret_cast<Executor*>(exe);
		meshWithPositionUseSubresourceHelper(request, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset,
			executor->streamingManager, executor->meshAllocator, executor->sharedResources->meshManager, executor->sharedResources->graphicsEngine.graphicsDevice);
	}

	static void meshWithPositionUseSubresourceHelper(RamToVramUploadRequest* request, void* const uploadBufferCpuAddressOfCurrentPos,
		ID3D12Resource* uploadResource, uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager, FixedSizeAllocator<Mesh>& meshAllocator,
		MeshManager& meshManager, ID3D12Device* graphicsDevice);


	static void CalculateTangentBitangent(unsigned char* start, unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh);

	static Mesh* createMesh(FixedSizeAllocator<Mesh>& meshAllocator, const wchar_t* filename, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t vertexStrideInBytes,
		uint32_t indexSizeBytes);
public:
	MeshManager();
	~MeshManager();
	template<class Executor>
	static void loadMeshWithPositionTextureNormalTangentBitangent(const wchar_t * filename, void* requester, Executor* const executor,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, Mesh* mesh))
	{
		loadMesh(filename, requester, executor, resourceUploadedCallback, meshWithPositionTextureNormalTangentBitangentUseSubresource<Executor>,
			sizeof(MeshWithPositionTextureNormalTangentBitangent), sizeof(MeshWithPositionTextureNormal), executor->streamingManager);
	}

	template<class Executor>
	static void loadMeshWithPositionTextureNormal(const wchar_t * filename, void* requester, Executor* const executor,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, Mesh* mesh))
	{
		loadMesh(filename, requester, executor, resourceUploadedCallback, meshWithPositionTextureNormalUseSubresource<Executor>,
			sizeof(MeshWithPositionTextureNormal), sizeof(MeshWithPositionTextureNormal), executor->streamingManager);
	}

	template<class Executor>
	static void loadMeshWithPositionTexture(const wchar_t * filename, void* requester, Executor* const executor,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, Mesh* mesh))
	{
		loadMesh(filename, requester, executor, resourceUploadedCallback, meshWithPositionTextureUseSubresource<Executor>,
			sizeof(MeshWithPositionTexture), sizeof(MeshWithPositionTexture), executor->streamingManager);
	}

	template<class Executor>
	static void loadMeshWithPosition(const wchar_t * filename, void* requester, Executor* const executor,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, Mesh* mesh))
	{
		loadMesh(filename, requester, executor, resourceUploadedCallback, meshWithPositionUseSubresource<Executor>,
			sizeof(MeshWithPosition), sizeof(MeshWithPosition), executor->streamingManager);
	}

	void unloadMesh(const wchar_t * const filename, BaseExecutor* const executor);
};