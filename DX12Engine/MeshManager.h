#pragma once
#include "Mesh.h"
#include "RamToVramUploadRequest.h"
#include "ResizingArray.h"
#include <unordered_map>
#include <mutex>
#include "FixedSizeAllocator.h"
#include "Delegate.h"
#include "StreamingManager.h"
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

	using Request = Delegate<void(void* executor, void* sharedResources, Mesh* mesh)>;

	struct MeshInfo
	{
		MeshInfo() : numUsers(0u), loaded(false) {}
		unsigned int numUsers;
		bool loaded;
		Mesh mesh;
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, ResizingArray<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, MeshInfo, std::hash<const wchar_t *>> meshInfos;

	static void meshUploadedHelper(MeshManager& MeshManager, void* storedFilename, void* executor, void* sharedResources);

	template<class GlobalResources>
	static void meshUploaded(void* storedFilename, void* executor, void* sharedResources)
	{
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
		MeshManager& meshManager = globalResources.meshManager;
		meshUploadedHelper(meshManager, storedFilename, executor, sharedResources);
	}

	static void meshWithPositionTextureNormalTangentBitangentUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, unsigned int threadIndex);
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionTextureNormalTangentBitangentUseSubresource(void* tr, void* gr, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
			auto& uploadRequest = *useSubresourceRequest.uploadRequest;
			wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
			auto sizeOnFile = uploadRequest.meshInfo.verticesSize;

			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, filename, sizeof(uint32_t) * 3u, sizeof(uint32_t) * 3u + sizeOnFile, uploadRequest.file, &useSubresourceRequest,
				[](void* requester, void* tr, void* gr, const unsigned char* buffer, File file)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionTextureNormalTangentBitangentUseSubresourceHelper(*reinterpret_cast<HalfFinishedUploadRequest*>(requester), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, threadResources.taskShedular.index());
			});
		} });
	}

	static void meshWithPositionTextureNormalUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, unsigned int threadIndex);
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionTextureNormalUseSubresource(void* tr, void* gr, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
			auto& uploadRequest = *useSubresourceRequest.uploadRequest;
			wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
			auto sizeOnFile = uploadRequest.meshInfo.verticesSize;

			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, filename, sizeof(uint32_t) * 3u, sizeof(uint32_t) * 3u + sizeOnFile, uploadRequest.file, &useSubresourceRequest,
				[](void* requester, void* tr, void* gr, const unsigned char* buffer, File file)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionTextureNormalUseSubresourceHelper(*reinterpret_cast<HalfFinishedUploadRequest*>(requester), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, threadResources.taskShedular.index());
			});
		} });
	}

	static void meshWithPositionTextureUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, unsigned int threadIndex);
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionTextureUseSubresource(void* tr, void* gr, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
			auto& uploadRequest = *useSubresourceRequest.uploadRequest;
			wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
			auto sizeOnFile = uploadRequest.meshInfo.verticesSize;

			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, filename, sizeof(uint32_t) * 3u, sizeof(uint32_t) * 3u + sizeOnFile, uploadRequest.file, &useSubresourceRequest,
				[](void* requester, void* tr, void* gr, const unsigned char* buffer, File file)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionTextureUseSubresourceHelper(*reinterpret_cast<HalfFinishedUploadRequest*>(requester), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, threadResources.taskShedular.index());
			});
		} });
	}

	static void meshWithPositionUseSubresourceHelper(HalfFinishedUploadRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, unsigned int threadIndex);
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionUseSubresource(void* tr, void* gr, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
			auto& uploadRequest = *useSubresourceRequest.uploadRequest;
			wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
			auto sizeOnFile = uploadRequest.meshInfo.verticesSize;

			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, filename, sizeof(uint32_t) * 3u, sizeof(uint32_t) * 3u + sizeOnFile, uploadRequest.file, &useSubresourceRequest,
				[](void* requester, void* tr, void* gr, const unsigned char* buffer, File file)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionUseSubresourceHelper(*reinterpret_cast<HalfFinishedUploadRequest*>(requester), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, threadResources.taskShedular.index());
			});
		} });
	}

	static void fillUploadRequest(RamToVramUploadRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t vertexStride, void* filename, File file, void(*meshUploaded)(void* storedFilename, void* executor, void* sharedResources));

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTextureNormalTangentBitangent>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f_normal3f)
			{
				uploadRequest.useSubresource = meshWithPositionTextureNormalTangentBitangentUseSubresource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTextureNormal>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f_normal3f)
			{
				uploadRequest.useSubresource = meshWithPositionTextureNormalUseSubresource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTexture>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f)
			{
				uploadRequest.useSubresource = meshWithPositionTextureUseSubresource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPosition>, void> fillUploadRequestUseSubresourceCallback(RamToVramUploadRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f)
			{
				uploadRequest.useSubresource = meshWithPositionUseSubresource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	void loadMeshUncached(ThreadResources& executor, GlobalResources& sharedResources, const wchar_t* filename)
	{
		File file = sharedResources.asynchronousFileManager.openFileForReading<GlobalResources>(sharedResources.ioCompletionQueue, filename);
		sharedResources.asynchronousFileManager.readFile(&executor, &sharedResources, filename, 0u, sizeof(uint32_t) * 3u, file, reinterpret_cast<void*>(const_cast<wchar_t *>(filename)),
			[](void* requester, void* executor, void* sharedResources, const unsigned char* buffer, File file)
		{
			const uint32_t* data = reinterpret_cast<const uint32_t*>(buffer);
			uint32_t vertexType2 = data[0];
			uint32_t vertexCount = data[1];
			uint32_t indexCount = data[2];
			RamToVramUploadRequest uploadRequest;
			fillUploadRequest(uploadRequest, vertexCount, indexCount, sizeof(VertexType_t), requester, file, meshUploaded<GlobalResources>);
			fillUploadRequestUseSubresourceCallback<VertexType_t, ThreadResources, GlobalResources>(uploadRequest, indexCount, vertexType2);
			GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
			globalResources.streamingManager.addUploadRequest(uploadRequest);
		});
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	void loadMesh(const wchar_t * filename, void* requester, ThreadResources& executor, GlobalResources& sharedResources,
		void(*resourceUploadedCallback)(void* requester, void* executor, void* sharedResources, Mesh* mesh))
	{
		std::unique_lock<decltype(mutex)> lock(mutex);
		MeshInfo& meshInfo = meshInfos[filename];
		meshInfo.numUsers += 1u;
		if (meshInfo.numUsers == 1u)
		{
			uploadRequests[filename].push_back(Request{ requester, resourceUploadedCallback });
			lock.unlock();

			loadMeshUncached<VertexType_t>(executor, sharedResources, filename);
		}
		else if (meshInfo.loaded)
		{
			Mesh* mesh = &meshInfo.mesh;
			lock.unlock();
			resourceUploadedCallback(requester, &executor, &sharedResources, mesh);
		}
		else
		{
			uploadRequests[filename].push_back(Request{ requester, resourceUploadedCallback });
		}
	}

	static void CalculateTangentBitangent(const unsigned char* start, const unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh);

	static void createMesh(Mesh& mesh, const wchar_t* filename, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t vertexStrideInBytes,
		uint32_t indexSizeBytes);
public:
	MeshManager();
	~MeshManager();
	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPositionTextureNormalTangentBitangent(const wchar_t * filename, void* requester, ThreadResources& executor, GlobalResources& sharedResources,
		void(*resourceUploadedCallback)(void* requester, void* executor, void* sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPositionTextureNormalTangentBitangent, ThreadResources, GlobalResources>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPositionTextureNormal(const wchar_t * filename, void* requester, ThreadResources& executor, GlobalResources& sharedResources,
		void(*resourceUploadedCallback)(void* requester, void* executor, void* sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPositionTextureNormal, ThreadResources, GlobalResources>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPositionTexture(const wchar_t * filename, void* requester, ThreadResources& executor, GlobalResources& sharedResources,
		void(*resourceUploadedCallback)(void* requester, void* executor, void* sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPositionTexture, ThreadResources, GlobalResources>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPosition(const wchar_t * filename, void* requester, ThreadResources& executor, GlobalResources& sharedResources,
		void(*resourceUploadedCallback)(void* requester, void* executor, void* sharedResources, Mesh* mesh))
	{
		loadMesh<MeshWithPosition, ThreadResources, GlobalResources>(filename, requester, executor, sharedResources, resourceUploadedCallback);
	}

	void unloadMesh(const wchar_t * const filename);
};