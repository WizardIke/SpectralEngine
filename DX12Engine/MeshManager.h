#pragma once
#include "Mesh.h"
#include <unordered_map>
#include "StreamingManager.h"
#include "File.h"
#include "AsynchronousFileManager.h"
#include "ActorQueue.h"

class MeshManager
{
	enum class Action : short
	{
		loadMesh,
		unloadMesh,
		notifyMeshReady,
	};
public:
	class Messgage : public AsynchronousFileManager::ReadRequest
	{
	public:
		Action action;
	};

	class MeshStreamingRequest : public StreamingManager::StreamingRequest, public Messgage
	{
	public:
		constexpr static unsigned int numberOfComponents = 3u; //mesh header, mesh data, stream to gpu request
		std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;
		uint32_t sizeOnFile;
		uint32_t verticesSize;
		uint32_t indicesSize;
		uint32_t vertexStride;

		ID3D12Resource* vertices;
		ID3D12Resource* indices;
		ID3D12Heap* meshBuffer;

		MeshStreamingRequest* nextMeshRequest;

		void(*meshLoaded)(MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh);
		void(*deleteMeshRequest)(MeshStreamingRequest& request);

		MeshStreamingRequest(void(*meshLoaded)(MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh),
			void(*deleteMeshRequest)(MeshStreamingRequest& request),
			const wchar_t * filename) :
			meshLoaded(meshLoaded),
			deleteMeshRequest(deleteMeshRequest)
		{
			this->filename = filename;
		}
	};
private:
	class MeshHeader
	{
	public:
		uint32_t compressedVertexType;
		uint32_t unpackedVertexType;
		uint32_t vertexCount;
		uint32_t indexCount;
	};

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
		MeshInfo() : numUsers(0u), lastRequest(nullptr) {}
		Mesh mesh;
		MeshStreamingRequest* lastRequest;
		unsigned int numUsers;
	};
	ActorQueue messageQueue;
	std::unordered_map<const wchar_t* const, MeshInfo, std::hash<const wchar_t*>> meshInfos;

	template<class ThreadResources, class GlobalResources>
	void addMessage(SinglyLinked* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.backgroundQueue().push({this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& manager = *static_cast<MeshManager*>(requester);
				manager.run(threadResources, globalResources);
			}});
		}
	}

	void notifyMeshReadyHelper(MeshStreamingRequest* request, void* executor, void* sharedResources);

	template<class ThreadResources, class GlobalResources>
	void notifyMeshReady(MeshStreamingRequest* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		notifyMeshReadyHelper(request, &threadResources, &globalResources);
		StreamingManager& streamingManager = globalResources.streamingManager;
		streamingManager.uploadFinished(request, threadResources, globalResources);
	}

	static void freeRequestMemory(StreamingManager::StreamingRequest* request, void*, void*);

	template<class ThreadResources, class GlobalResources>
	static void copyFinished(void* requester, void* tr, void* gr)
	{
		MeshStreamingRequest* request = static_cast<MeshStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

		MeshManager& meshManager = globalResources.meshManager;
		request->action = Action::notifyMeshReady;
		meshManager.addMessage(request, threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources, void(*useResourceHelper)(MeshStreamingRequest&, const unsigned char*, ID3D12Device*,
		StreamingManager::ThreadLocal&, void(*copyFinished)(void*, void*, void*))>
	static void useResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr, void*)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *static_cast<MeshStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(requester));
			const auto sizeOnFile = uploadRequest.sizeOnFile;

			uploadRequest.start = sizeof(MeshHeader);
			uploadRequest.end = sizeof(MeshHeader) + sizeOnFile;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				useResourceHelper(static_cast<MeshStreamingRequest&>(request), buffer, globalResources.graphicsEngine.graphicsDevice,
					threadResources.streamingManager, copyFinished<ThreadResources, GlobalResources>);
				globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
			};
			globalResources.asynchronousFileManager.readFile(&uploadRequest, threadResources, globalResources);
		}});
	}

	static void meshWithPositionTextureNormalTangentBitangentUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr, void* gr));
	static void meshWithPositionTextureNormalUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr, void* gr));
	static void meshWithPositionTextureUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr, void* gr));
	static void meshWithPositionUseResourceHelper(MeshStreamingRequest& useSubresourceRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr, void* gr));
	
	static void fillUploadRequestHelper(MeshStreamingRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t vertexStride);

	template<class ThreadResources, class GlobalResources>
	static void fillUploadRequest(MeshStreamingRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t compressedVertexType, uint32_t unpackedVertexType)
	{
		switch(unpackedVertexType) 
		{
		case VertexType::position3f_texCoords2f_normal3f_tangent3f_bitangent3f:
			if(indexCount == 0u)
			{
				if(compressedVertexType == VertexType::position3f_texCoords2f_normal3f)
				{
					uploadRequest.streamResource = useResource<ThreadResources, GlobalResources, meshWithPositionTextureNormalTangentBitangentUseResourceHelper>;
				}
			}
			fillUploadRequestHelper(uploadRequest, vertexCount, indexCount, sizeof(MeshWithPositionTextureNormalTangentBitangent));
			break;
		case VertexType::position3f_texCoords2f_normal3f:
			if(indexCount == 0u)
			{
				if(compressedVertexType == VertexType::position3f_texCoords2f_normal3f)
				{
					uploadRequest.streamResource = useResource<ThreadResources, GlobalResources, meshWithPositionTextureNormalUseResourceHelper>;
				}
			}
			fillUploadRequestHelper(uploadRequest, vertexCount, indexCount, sizeof(MeshWithPositionTextureNormal));
			break;
		case VertexType::position3f_texCoords2f:
			if(indexCount == 0u)
			{
				if(compressedVertexType == VertexType::position3f_texCoords2f)
				{
					uploadRequest.streamResource = useResource<ThreadResources, GlobalResources, meshWithPositionTextureUseResourceHelper>;
				}
			}
			fillUploadRequestHelper(uploadRequest, vertexCount, indexCount, sizeof(MeshWithPositionTexture));
			break;
		case VertexType::position3f:
			if(indexCount == 0u)
			{
				if(compressedVertexType == VertexType::position3f)
				{
					uploadRequest.streamResource = useResource<ThreadResources, GlobalResources, meshWithPositionUseResourceHelper>;
				}
			}
			fillUploadRequestHelper(uploadRequest, vertexCount, indexCount, sizeof(MeshWithPosition));
			break;
		}
	}

	template<class ThreadResources, class GlobalResources>
	void loadMeshUncached(ThreadResources& threadResources, GlobalResources& globalResources, MeshStreamingRequest& request)
	{
		request.file = globalResources.asynchronousFileManager.openFileForReading<GlobalResources>(globalResources.ioCompletionQueue, request.filename);
		request.start = 0u;
		request.end = sizeof(MeshHeader);
		request.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* buffer)
		{
			MeshStreamingRequest& uploadRequest = static_cast<MeshStreamingRequest&>(request);
			const MeshHeader* header = reinterpret_cast<const MeshHeader*>(buffer);
			const uint32_t compressedVertexType = header->compressedVertexType;
			const uint32_t unpackedVertexType = header->unpackedVertexType;
			const uint32_t vertexCount = header->vertexCount;
			const uint32_t indexCount = header->indexCount;

			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);

			fillUploadRequest<ThreadResources, GlobalResources>(uploadRequest, vertexCount, indexCount, compressedVertexType, unpackedVertexType);
			globalResources.streamingManager.addUploadRequest(&uploadRequest, threadResources, globalResources);
		};
		request.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			freeRequestMemory(static_cast<MeshStreamingRequest*>(&request), tr, gr);
		};
		globalResources.asynchronousFileManager.readFile(&request, threadResources, globalResources);
	}

	static void createMeshResources(ID3D12Resource*& vertices, ID3D12Resource*& indices, ID3D12Heap*& meshBuffer, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t indexSizeBytes
#ifndef ndebug
		, const wchar_t* filename
#endif
	);
	static void CalculateTangentBitangent(const unsigned char* start, const unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh);
	static void fillMesh(Mesh& mesh, MeshStreamingRequest& request);

	template<class ThreadResources, class GlobalResources>
	void loadMesh(ThreadResources& executor, GlobalResources& sharedResources, MeshStreamingRequest* request)
	{
		MeshInfo& meshInfo = meshInfos[request->filename];
		meshInfo.numUsers += 1u;
		if(meshInfo.numUsers == 1u)
		{
			meshInfo.lastRequest = request;
			request->nextMeshRequest = nullptr;

			loadMeshUncached(executor, sharedResources, *request);
			return;
		}
		//the resource is loaded or loading
		if(meshInfo.lastRequest == nullptr)
		{
			//The resource is loaded
			request->meshLoaded(*request, &executor, &sharedResources, meshInfo.mesh);
			return;
		}
		meshInfo.lastRequest->nextMeshRequest = request;
		meshInfo.lastRequest = request;
		request->nextMeshRequest = nullptr;
	}

	void unloadMesh(const wchar_t* const filename);

	template<class ThreadResources, class GlobalResources>
	void run(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		do
		{
			SinglyLinked* temp = messageQueue.popAll();
			for(; temp != nullptr;)
			{
				auto& message = *static_cast<Messgage*>(temp);
				temp = temp->next; //Allow reuse of next
				if(message.action == Action::unloadMesh)
				{
					unloadMesh(message.filename);
				}
				else if(message.action == Action::loadMesh)
				{
					loadMesh(static_cast<MeshStreamingRequest*>(&message), threadResources, globalResources);
				}
				else
				{
					notifyMeshReady(static_cast<MeshStreamingRequest*>(&message), threadResources, globalResources);
				}
			}
		} while(!messageQueue.stop());
	}
public:
	MeshManager();
	~MeshManager();

	template<class ThreadResources, class GlobalResources>
	void load(MeshStreamingRequest* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->action = Action::loadMesh;
		addMessage(request, threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources>
	void unload(Messgage* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->action = Action::unloadMesh;
		addMessage(request, threadResources, globalResources);
	}
};