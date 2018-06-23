#pragma once
#include "Mesh.h"
#include "RamToVramUploadRequest.h"
#include "ResizingArray.h"
#include <unordered_map>
#include <mutex>
#include "FixedSizeAllocator.h"
#include "Delegate.h"
class BaseExecutor;
class StreamingManager;
class HalfFinishedUploadRequest;

class MeshManager
{
	enum VertexType : uint32_t
	{
		position3f = 0u,
		position3f_texCoords2f = 1u,
		position3f_texCoords2f_normal3f = 2u,
		position3f_texCoords2f_normal3f_tangent3f_bitangent3f = 3u,
		position3f_color3f = 4u,
		position3f_color4f = 5u,
	};

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

	using Request = Delegate<void(BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)>;

	struct MeshInfo
	{
		MeshInfo() : numUsers(0u), loaded(false) {}
		Mesh* mesh;
		unsigned int numUsers;
		bool loaded;
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, ResizingArray<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, MeshInfo, std::hash<const wchar_t *>> meshInfos;

	static void meshWithPositionTextureNormalTangentBitangentUseSubresource(BaseExecutor* exe, SharedResources& sr, HalfFinishedUploadRequest& useSubresourceRequest);
	static void MeshManager::meshWithPositionTextureNormalTangentBitangentUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, SharedResources& sharedResources, BaseExecutor* executor);

	static void meshWithPositionTextureNormalUseSubresource(BaseExecutor* exe, SharedResources& sr, HalfFinishedUploadRequest& useSubresourceRequest);
	static void meshWithPositionTextureNormalUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, SharedResources& sharedResources, BaseExecutor* executor);

	static void meshWithPositionTextureUseSubresource(BaseExecutor* exe, SharedResources& sr, HalfFinishedUploadRequest& useSubresourceRequest);
	static void meshWithPositionTextureUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, SharedResources& sharedResources, BaseExecutor* executor);

	static void meshWithPositionUseSubresource(BaseExecutor* exe, SharedResources& sr, HalfFinishedUploadRequest& useSubresourceRequest);
	static void meshWithPositionUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, SharedResources& sharedResources, BaseExecutor* executor);

	static void fillUploadRequest(RamToVramUploadRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t vertexStride, void* filename, File file);

	template<class VertexType_t>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTextureNormalTangentBitangent>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f_normal3f)
			{
				uploadRequest.useSubresource = meshWithPositionTextureNormalTangentBitangentUseSubresource;
			}
		}
	}

	template<class VertexType_t>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTextureNormal>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f_normal3f)
			{
				uploadRequest.useSubresource = meshWithPositionTextureNormalUseSubresource;
			}
		}
	}

	template<class VertexType_t>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTexture>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f)
			{
				uploadRequest.useSubresource = meshWithPositionTextureUseSubresource;
			}
		}
	}

	template<class VertexType_t>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPosition>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f)
			{
				uploadRequest.useSubresource = meshWithPositionUseSubresource;
			}
		}
	}

	template<class VertexType_t, class Executor_t, class SharedResources_t>
	void loadMeshUncached(Executor_t executor, SharedResources_t& sharedResources, const wchar_t* filename)
	{
		File file = sharedResources.asynchronousFileManager.openFileForReading(sharedResources.ioCompletionQueue, filename);
		sharedResources.asynchronousFileManager.readFile(executor, sharedResources, filename, 0u, sizeof(uint32_t) * 3u, file, reinterpret_cast<void*>(const_cast<wchar_t *>(filename)),
			[](void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* buffer, File file)
		{
			const uint32_t* data = reinterpret_cast<const uint32_t*>(buffer);
			uint32_t vertexType2 = data[0];
			uint32_t vertexCount = data[1];
			uint32_t indexCount = data[2];
			RamToVramUploadRequest uploadRequest;
			fillUploadRequest(uploadRequest, vertexCount, indexCount, sizeof(VertexType_t), requester, file);
			fillUploadRequestUseSubresourceCallback<VertexType_t>(uploadRequest, indexCount, vertexType2);
			sharedResources.streamingManager.addUploadRequest(uploadRequest);
		});
	}

	template<class VertexType_t, class Executor_t, class SharedResources_t>
	static void loadMesh(const wchar_t * filename, void* requester, BaseExecutor* const executor, SharedResources& sharedResources,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources, Mesh* mesh))
	{
		MeshManager& meshManager = sharedResources.meshManager;
		std::unique_lock<decltype(meshManager.mutex)> lock(meshManager.mutex);
		MeshInfo& meshInfo = meshManager.meshInfos[filename];
		meshInfo.numUsers += 1u;
		if (meshInfo.numUsers == 1u)
		{
			meshManager.uploadRequests[filename].push_back(Request{ requester, resourceUploadedCallback });
			lock.unlock();

			meshManager.loadMeshUncached<VertexType_t>(executor, sharedResources, filename);
		}
		else if (meshInfo.loaded)
		{
			Mesh* mesh = meshInfo.mesh;
			lock.unlock();
			resourceUploadedCallback(requester, executor, sharedResources, mesh);
		}
		else
		{
			meshManager.uploadRequests[filename].push_back(Request{ requester, resourceUploadedCallback });
		}
	}

	static void meshUploaded(void* storedFilename, BaseExecutor* executor, SharedResources& sharedResources);

	static void CalculateTangentBitangent(const uint8_t* start, const uint8_t* end, MeshWithPositionTextureNormalTangentBitangent* Mesh);

	static Mesh* createMesh(FixedSizeAllocator<Mesh>& meshAllocator, const wchar_t* filename, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t vertexStrideInBytes,
		uint32_t indexSizeBytes);
public:
	MeshManager();
	~MeshManager();
	template<class Executor, class SharedResources_t>
	static void loadMeshWithPositionTextureNormalTangentBitangent(const wchar_t * filename, void* requester, Executor* const executor, SharedResources_t& sharedResources,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPositionTextureNormalTangentBitangent, Executor, SharedResources_t>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	template<class Executor, class SharedResources_t>
	static void loadMeshWithPositionTextureNormal(const wchar_t * filename, void* requester, Executor* const executor, SharedResources_t& sharedResources,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPositionTextureNormal, Executor, SharedResources_t>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	template<class Executor, class SharedResources_t>
	static void loadMeshWithPositionTexture(const wchar_t * filename, void* requester, Executor* const executor, SharedResources_t& sharedResources,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPositionTexture, Executor, SharedResources_t>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	template<class Executor, class SharedResources_t>
	static void loadMeshWithPosition(const wchar_t * filename, void* requester, Executor* const executor, SharedResources_t& sharedResources,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPosition, Executor, SharedResources_t>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	void unloadMesh(const wchar_t * const filename, BaseExecutor* const executor);
};