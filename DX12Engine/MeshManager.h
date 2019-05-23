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
		load,
		unload,
		notifyReady,
	};
public:
	class Message : public AsynchronousFileManager::ReadRequest
	{
	public:
		Action meshAction;

		Message() {}
		Message(const wchar_t* filename, void(*deleteRequest)(ReadRequest& request, void* tr, void* gr))
		{
			this->filename = filename;
			this->deleteReadRequest = deleteRequest;
		}
	};

	class MeshStreamingRequest : public StreamingManager::StreamingRequest, public Message
	{
	public:
		uint32_t sizeOnFile;
		uint32_t verticesSize;
		uint32_t indicesSize;
		uint32_t vertexStride;

		ID3D12Resource* vertices;
		ID3D12Resource* indices;
		ID3D12Heap* meshBuffer;

		MeshStreamingRequest* nextMeshRequest;

		void(*meshLoaded)(MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh);

		MeshStreamingRequest(void(*meshLoaded)(MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh),
			const wchar_t * filename) :
			meshLoaded(meshLoaded)
		{
			this->filename = filename;
		}
	};

	using UnloadRequest = Message;
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
	void addMessage(Message* request, ThreadResources& threadResources, GlobalResources&)
	{
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& manager = *static_cast<MeshManager*>(requester);
				manager.run(threadResources, globalResources);
			}});
		}
	}

	void notifyMeshReady(MeshStreamingRequest* request, void* executor, void* sharedResources);

	template<class ThreadResources, class GlobalResources>
	static void copyFinished(void* requester, void* tr, void* gr)
	{
		MeshStreamingRequest* request = static_cast<MeshStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

		request->deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request1, void* tr, void* gr)
		{
			MeshStreamingRequest* request = static_cast<MeshStreamingRequest*>(&request1);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

			request->deleteStreamingRequest = [](StreamingManager::StreamingRequest* request1, void* tr, void* gr)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				MeshManager& meshManager = globalResources.meshManager;
				auto request = static_cast<MeshStreamingRequest*>(request1);
				request->meshAction = Action::notifyReady;
				meshManager.addMessage(request, threadResources, globalResources);
			};
			StreamingManager& streamingManager = globalResources.streamingManager;
			streamingManager.uploadFinished(request, threadResources, globalResources);
		};
		globalResources.asynchronousFileManager.discard(request, threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources, void(*useResourceHelper)(MeshStreamingRequest&, const unsigned char*, ID3D12Device*,
		StreamingManager::ThreadLocal&, void(*copyFinished)(void*, void*, void*))>
	static void useResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr, void*)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.pushBackgroundTask({useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
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
			fillUploadRequest<ThreadResources, GlobalResources>(uploadRequest, header->vertexCount, header->indexCount, header->compressedVertexType, header->unpackedVertexType);

			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
		};
		request.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			StreamingManager& streamingManager = globalResources.streamingManager;
			streamingManager.addUploadRequest(static_cast<MeshStreamingRequest*>(&request), threadResources, globalResources);
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
	void loadMesh(MeshStreamingRequest* request, ThreadResources& executor, GlobalResources& sharedResources)
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

	void unloadMesh(UnloadRequest& unloadRequest, void* tr, void* gr);

	template<class ThreadResources, class GlobalResources>
	void run(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		do
		{
			SinglyLinked* temp = messageQueue.popAll();
			for(; temp != nullptr;)
			{
				auto& message = *static_cast<Message*>(temp);
				temp = temp->next; //Allow reuse of next
				if(message.meshAction == Action::unload)
				{
					unloadMesh(static_cast<UnloadRequest&>(message), &threadResources, &globalResources);
				}
				else if(message.meshAction == Action::load)
				{
					loadMesh(static_cast<MeshStreamingRequest*>(&message), threadResources, globalResources);
				}
				else
				{
					notifyMeshReady(static_cast<MeshStreamingRequest*>(&message), &threadResources, &globalResources);
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
		request->meshAction = Action::load;
		addMessage(request, threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources>
	void unload(Message* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->meshAction = Action::unload;
		addMessage(request, threadResources, globalResources);
	}
};