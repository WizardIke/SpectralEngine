#pragma once
#include "Mesh.h"
#include "RamToVramUploadRequest.h"
#include <vector>
#include <unordered_map>
#include <mutex>
class BaseExecutor;

class MeshManager
{
	struct MeshWithPositionTextureNormalTangentBitangent
	{
		float x, y, z, w;
		float tu, tv;
		float nx, ny, nz;
		float tx, ty, tz;
		float bx, by, bz;
	};

	struct MeshWithPositionTextureNormal
	{
		float x, y, z, w;
		float tu, tv;
		float nx, ny, nz;
	};

	struct MeshWithPositionTexture
	{
		float x, y, z, w;
		float tu, tv;
	};

	struct PendingLoadRequest
	{
		void* requester;
		void(*resourceUploaded)(void* const requester, BaseExecutor* const executor);
		Mesh** destination;
	};

	struct MeshInfo
	{
		MeshInfo() : numUsers(0u), loaded(false) {}
		Mesh* mesh;
		unsigned int numUsers;
		bool loaded; //remove this bool
	};

	struct MeshHeader
	{
		uint32_t numVertices;
		uint32_t numIndices;
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, std::vector<PendingLoadRequest>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, MeshInfo, std::hash<const wchar_t *>> meshInfos;

	static void loadMesh(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor),
		void(*useSubresourceCallback)(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset), uint32_t vertexStrideInBytes, Mesh** destination,
		uint32_t vertexPackedSize);

	void loadMeshUncached(BaseExecutor* const executor, const wchar_t * filename, void(*useSubresourceCallback)(RamToVramUploadRequest* request, BaseExecutor* executor,
		void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset), uint32_t vertexStrideInBytes, uint32_t vertexPackedSize);

	static void meshUploaded(void* storedFilename, BaseExecutor* executor);

	static void meshWithPositionTextureNormalTangentBitangentUseSubresource(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset);

	static void meshWithPositionTextureNormalUseSubresource(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset);

	static void meshWithPositionTextureUseSubresource(RamToVramUploadRequest* request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset);


	static void CalculateTangentBitangent(unsigned char* start, unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh);
public:
	MeshManager();
	~MeshManager();
	static void loadMeshWithPositionTextureNormalTangentBitangent(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor), Mesh** destination)
	{
		loadMesh(filename, requester, executor, resourceUploadedCallback, meshWithPositionTextureNormalTangentBitangentUseSubresource, sizeof(MeshWithPositionTextureNormalTangentBitangent), destination, sizeof(float) * 8u);
	}

	static void loadMeshWithPositionTextureNormal(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor), Mesh** destination)
	{
		loadMesh(filename, requester, executor, resourceUploadedCallback, meshWithPositionTextureNormalUseSubresource, sizeof(MeshWithPositionTextureNormal), destination, sizeof(float) * 8u);
	}

	static void loadMeshWithPositionTexture(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor), Mesh** destination)
	{
		loadMesh(filename, requester, executor, resourceUploadedCallback, meshWithPositionTextureUseSubresource, sizeof(MeshWithPositionTexture), destination, sizeof(float) * 5u);
	}

	void unloadMesh(const wchar_t * const filename, BaseExecutor* const executor);
};