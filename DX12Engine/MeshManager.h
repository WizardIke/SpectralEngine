#pragma once
#include "Mesh.h"
#include <unordered_map>
#include <mutex>
#include "StreamingManager.h"
#include "File.h"
#include "AsynchronousFileManager.h"

class MeshManager
{
public:
	class MeshStreamingRequest : public StreamingManager::StreamingRequest, public AsynchronousFileManager::IORequest
	{
	public:
		uint32_t sizeOnFile;
		uint32_t verticesSize;
		uint32_t indicesSize;

		void(*meshLoaded)(MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh);
		MeshStreamingRequest* nextMeshRequest;

		MeshStreamingRequest(void(*meshLoaded)(MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh),
			const wchar_t * filename) :
			meshLoaded(meshLoaded)
		{
			this->filename = filename;
		}
	};
private:
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

	struct MeshInfo
	{
		MeshInfo() : numUsers(0u), loaded(false) {}
		unsigned int numUsers;
		bool loaded;
		Mesh mesh;
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, MeshStreamingRequest*, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, MeshInfo, std::hash<const wchar_t *>> meshInfos;

	static void meshUploadedHelper(MeshManager& MeshManager, const wchar_t* storedFilename, void* executor, void* sharedResources);

	template<class GlobalResources>
	static void meshUploaded(StreamingManager::StreamingRequest* request, void* executor, void* sharedResources)
	{
		GlobalResources& globalResources = *static_cast<GlobalResources*>(sharedResources);
		MeshManager& meshManager = globalResources.meshManager;
		meshUploadedHelper(meshManager, static_cast<MeshStreamingRequest*>(request)->filename, executor, sharedResources);
	}

	template<class ThreadResources, class GlobalResources>
	static void copyStarted(void* requester, void* tr, void* gr)
	{
		MeshStreamingRequest* request = static_cast<MeshStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
		StreamingManager& streamingManager = globalResources.streamingManager;
		streamingManager.uploadFinished(request, threadResources, globalResources);
	}

	static void meshWithPositionTextureNormalTangentBitangentUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyStarted)(void* requester, void* tr, void* gr));
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionTextureNormalTangentBitangentUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr, void* gr)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *static_cast<MeshStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(requester));
			const auto sizeOnFile = uploadRequest.sizeOnFile;

			uploadRequest.start = sizeof(uint32_t) * 3u;
			uploadRequest.end = sizeof(uint32_t) * 3u + sizeOnFile;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionTextureNormalTangentBitangentUseResourceHelper(static_cast<MeshStreamingRequest&>(request), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, copyStarted<ThreadResources, GlobalResources>);
			};
			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &uploadRequest);
		} });
	}

	static void meshWithPositionTextureNormalUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyStarted)(void* requester, void* tr, void* gr));
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionTextureNormalUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr, void*)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *static_cast<MeshStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(requester));
			const auto sizeOnFile = uploadRequest.sizeOnFile;

			uploadRequest.start = sizeof(uint32_t) * 3u;
			uploadRequest.end = sizeof(uint32_t) * 3u + sizeOnFile;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionTextureNormalUseResourceHelper(static_cast<MeshStreamingRequest&>(request), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, copyStarted<ThreadResources, GlobalResources>);
			};
			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &uploadRequest);
		} });
	}

	static void meshWithPositionTextureUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyStarted)(void* requester, void* tr, void* gr));
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionTextureUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr, void*)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *static_cast<MeshStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(requester));
			const auto sizeOnFile = uploadRequest.sizeOnFile;

			uploadRequest.start = sizeof(uint32_t) * 3u;
			uploadRequest.end = sizeof(uint32_t) * 3u + sizeOnFile;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionTextureUseResourceHelper(static_cast<MeshStreamingRequest&>(request), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, copyStarted<ThreadResources, GlobalResources>);
			};
			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &uploadRequest);
		} });
	}

	static void meshWithPositionUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, MeshManager& meshManager, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyStarted)(void* requester, void* tr, void* gr));
	template<class ThreadResources, class GlobalResources>
	static void meshWithPositionUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr, void*)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *static_cast<MeshStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(requester));
			const auto sizeOnFile = uploadRequest.sizeOnFile;

			uploadRequest.start = sizeof(uint32_t) * 3u;
			uploadRequest.end = sizeof(uint32_t) * 3u + sizeOnFile;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				meshWithPositionUseResourceHelper(static_cast<MeshStreamingRequest&>(request), buffer, globalResources.meshManager, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, copyStarted<ThreadResources, GlobalResources>);
			};
			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &uploadRequest);
		} });
	}

	static void fillUploadRequest(MeshStreamingRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t vertexStride,
		void(*meshUploaded)(StreamingManager::StreamingRequest* useSubresourceRequest, void* executor, void* sharedResources));

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTextureNormalTangentBitangent>, void> fillUploadRequestUseResourceCallback(MeshStreamingRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f_normal3f)
			{
				uploadRequest.streamResource = meshWithPositionTextureNormalTangentBitangentUseResource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTextureNormal>, void> fillUploadRequestUseResourceCallback(MeshStreamingRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f_normal3f)
			{
				uploadRequest.streamResource = meshWithPositionTextureNormalUseResource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPositionTexture>, void> fillUploadRequestUseResourceCallback(MeshStreamingRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f_texCoords2f)
			{
				uploadRequest.streamResource = meshWithPositionTextureUseResource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	static std::enable_if_t<std::is_same_v<VertexType_t, MeshWithPosition>, void> fillUploadRequestUseResourceCallback(MeshStreamingRequest& uploadRequest, uint32_t indexCount, uint32_t vertexType2)
	{
		if (indexCount == 0u)
		{
			if (vertexType2 == VertexType::position3f)
			{
				uploadRequest.streamResource = meshWithPositionUseResource<ThreadResources, GlobalResources>;
			}
		}
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	void loadMeshUncached(ThreadResources& threadResources, GlobalResources& globalResources, MeshStreamingRequest& request)
	{
		request.file = globalResources.asynchronousFileManager.openFileForReading<GlobalResources>(globalResources.ioCompletionQueue, request.filename);
		request.start = 0u;
		request.end = sizeof(uint32_t) * 3u;
		request.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
		{
			MeshStreamingRequest& uploadRequest = reinterpret_cast<MeshStreamingRequest&>(request);
			const uint32_t* data = reinterpret_cast<const uint32_t*>(buffer);
			uint32_t vertexType2 = data[0];
			uint32_t vertexCount = data[1];
			uint32_t indexCount = data[2];

			fillUploadRequest(uploadRequest, vertexCount, indexCount, sizeof(VertexType_t), meshUploaded<GlobalResources>);
			fillUploadRequestUseResourceCallback<VertexType_t, ThreadResources, GlobalResources>(uploadRequest, indexCount, vertexType2);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
			globalResources.streamingManager.addUploadRequest(&uploadRequest, threadResources, globalResources);
		};
		globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &request);
	}

	template<class VertexType_t, class ThreadResources, class GlobalResources>
	void loadMesh(ThreadResources& executor, GlobalResources& sharedResources, MeshStreamingRequest* request)
	{
		const wchar_t * filename = request->filename;
		std::unique_lock<decltype(mutex)> lock(mutex);
		MeshInfo& meshInfo = meshInfos[filename];
		meshInfo.numUsers += 1u;
		if (meshInfo.numUsers == 1u)
		{
			auto& requests = uploadRequests[filename];
			request->nextMeshRequest = requests;
			requests = request;
			lock.unlock();

			loadMeshUncached<VertexType_t>(executor, sharedResources, *request);
		}
		else if (meshInfo.loaded)
		{
			Mesh* mesh = &meshInfo.mesh;
			lock.unlock();
			request->meshLoaded(*request, &executor, &sharedResources, *mesh);
		}
		else
		{
			auto& requests = uploadRequests[filename];
			request->nextMeshRequest = requests;
			requests = request;
		}
	}

	static void CalculateTangentBitangent(const unsigned char* start, const unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh);

	static void createMesh(Mesh& mesh, const wchar_t* filename, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t vertexStrideInBytes,
		uint32_t indexSizeBytes);
public:
	MeshManager();
	~MeshManager();
	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPositionTextureNormalTangentBitangent(ThreadResources& executor, GlobalResources& sharedResources, MeshStreamingRequest* request)
	{
		loadMesh<MeshWithPositionTextureNormalTangentBitangent, ThreadResources, GlobalResources>(executor, sharedResources, request);
	}

	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPositionTextureNormal(ThreadResources& executor, GlobalResources& sharedResources, MeshStreamingRequest* request)
	{
		loadMesh<MeshWithPositionTextureNormal, ThreadResources, GlobalResources>(executor, sharedResources, request);
	}

	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPositionTexture(ThreadResources& executor, GlobalResources& sharedResources, MeshStreamingRequest* request)
	{
		loadMesh<MeshWithPositionTexture, ThreadResources, GlobalResources>(executor, sharedResources, request);
	}

	template<class ThreadResources, class GlobalResources>
	void loadMeshWithPosition(ThreadResources& executor, GlobalResources& sharedResources, MeshStreamingRequest* request)
	{
		loadMesh<MeshWithPosition, ThreadResources, GlobalResources>(executor, sharedResources, request);
	}

	void unloadMesh(const wchar_t * const filename);
};