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
		Message(const wchar_t* filename, void(*deleteRequest)(ReadRequest& request, void* tr))
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
		MeshManager* meshManager;

		MeshStreamingRequest* nextMeshRequest;

		void(*meshLoaded)(MeshStreamingRequest& request, void* tr, Mesh& mesh);

		MeshStreamingRequest(void(*meshLoaded)(MeshStreamingRequest& request, void* tr, Mesh& mesh),
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

	struct Hash
	{
		std::size_t operator()(const wchar_t* str) const noexcept
		{
			std::size_t result = 0u;
			while (*str != L'\0')
			{
				result = (result ^ std::size_t{ *str }) * static_cast<std::size_t>(16777619ul);
				++str;
			}
			return result;
		}
	};

	ActorQueue messageQueue;
	std::unordered_map<const wchar_t*, MeshInfo, Hash> meshInfos;
	AsynchronousFileManager& asynchronousFileManager;
	StreamingManager& streamingManager;
	ID3D12Device& graphicsDevice;

	template<class ThreadResources>
	void addMessage(Message* request, ThreadResources& threadResources)
	{
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources)
			{
				auto& manager = *static_cast<MeshManager*>(requester);
				manager.run(threadResources);
			}});
		}
	}

	void notifyMeshReady(MeshStreamingRequest* request, void* tr);

	template<class ThreadResources>
	static void copyFinished(void* requester, void*)
	{
		MeshStreamingRequest* request = static_cast<MeshStreamingRequest*>(requester);

		request->deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request1, void* tr)
		{
			MeshStreamingRequest* request = static_cast<MeshStreamingRequest*>(&request1);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);

			request->deleteStreamingRequest = [](StreamingManager::StreamingRequest* request1, void* tr)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				auto request = static_cast<MeshStreamingRequest*>(request1);
				MeshManager& meshManager = *request->meshManager;
				request->meshAction = Action::notifyReady;
				meshManager.addMessage(request, threadResources);
			};
			StreamingManager& streamingManager = request->meshManager->streamingManager;
			streamingManager.uploadFinished(request, threadResources);
		};
		request->asynchronousFileManager->discard(*request);
	}

	template<class ThreadResources, void(*useResourceHelper)(MeshStreamingRequest&, const unsigned char*, ID3D12Device*,
		StreamingManager::ThreadLocal&, void(*copyFinished)(void*, void*))>
	static void useResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.pushBackgroundTask({useSubresourceRequest, [](void* requester, ThreadResources&)
		{
			auto& uploadRequest = *static_cast<MeshStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(requester));
			const auto sizeOnFile = uploadRequest.sizeOnFile;

			uploadRequest.start = sizeof(MeshHeader);
			uploadRequest.end = sizeof(MeshHeader) + sizeOnFile;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager&, void* tr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				auto& uploadRequest = static_cast<MeshStreamingRequest&>(request);
				uploadRequest.file.close();
				useResourceHelper(uploadRequest, buffer, &uploadRequest.meshManager->graphicsDevice, threadResources.streamingManager, copyFinished<ThreadResources>);
			};
			uploadRequest.meshManager->asynchronousFileManager.readFile(uploadRequest);
		}});
	}

	static void meshWithPositionTextureNormalTangentBitangentUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr));
	static void meshNoConvertUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr));

	static void meshWithPositionTextureNormalTangentBitangentIndexUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr));
	static void meshNoConvertIndexUseResourceHelper(MeshStreamingRequest& uploadRequest, const unsigned char* buffer, ID3D12Device* graphicsDevice,
		StreamingManager::ThreadLocal& streamingManager, void(*copyFinished)(void* requester, void* tr));
	
	static void fillUploadRequestHelper(MeshStreamingRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t vertexStride);

	static uint32_t getVertexStride(uint32_t vertexType) noexcept;

	template<class ThreadResources>
	static void fillUploadRequest(MeshStreamingRequest& uploadRequest, uint32_t vertexCount, uint32_t indexCount, uint32_t compressedVertexType, uint32_t unpackedVertexType)
	{
		if (unpackedVertexType == compressedVertexType)
		{
			if (indexCount == 0u)
			{
				uploadRequest.streamResource = useResource<ThreadResources, meshNoConvertUseResourceHelper>;
			}
			else
			{
				uploadRequest.streamResource = useResource<ThreadResources, meshNoConvertIndexUseResourceHelper>;
			}
		}
		else
		{
			switch (unpackedVertexType)
			{
			case VertexType::position3f_texCoords2f_normal3f_tangent3f_bitangent3f:
				if (indexCount == 0u)
				{
					if (compressedVertexType == VertexType::position3f_texCoords2f_normal3f)
					{
						uploadRequest.streamResource = useResource<ThreadResources, meshWithPositionTextureNormalTangentBitangentUseResourceHelper>;
					}
					else
					{
						assert(false);
					}
				}
				else
				{
					if (compressedVertexType == VertexType::position3f_texCoords2f_normal3f)
					{
						uploadRequest.streamResource = useResource<ThreadResources, meshWithPositionTextureNormalTangentBitangentIndexUseResourceHelper>;
					}
					else
					{
						assert(false);
					}
				}
			default:
				assert(false);
			}
		}
		fillUploadRequestHelper(uploadRequest, vertexCount, indexCount, getVertexStride(unpackedVertexType));
	}

	template<class ThreadResources>
	void loadMeshUncached(MeshStreamingRequest& request)
	{
		request.meshManager = this;
		request.file = asynchronousFileManager.openFileForReading(request.filename);
		request.start = 0u;
		request.end = sizeof(MeshHeader);
		request.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* buffer)
		{
			MeshStreamingRequest& uploadRequest = static_cast<MeshStreamingRequest&>(request);
			const MeshHeader* header = reinterpret_cast<const MeshHeader*>(buffer);
			fillUploadRequest<ThreadResources>(uploadRequest, header->vertexCount, header->indexCount, header->compressedVertexType, header->unpackedVertexType);
			asynchronousFileManager.discard(request);
		};
		request.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			auto& uploadRequest = *static_cast<MeshStreamingRequest*>(&request);
			StreamingManager& streamingManager = uploadRequest.meshManager->streamingManager;
			streamingManager.addUploadRequest(&uploadRequest, threadResources);
		};
		asynchronousFileManager.readFile(request);
	}

	static void createMeshResources(ID3D12Resource*& vertices, ID3D12Resource*& indices, ID3D12Heap*& meshBuffer, ID3D12Device* graphicsDevice, uint32_t vertexSizeBytes, uint32_t indexSizeBytes
#ifndef NDEBUG
		, const wchar_t* filename
#endif
	);
	static void CalculateTangentBitangent(const unsigned char* start, const unsigned char* end, MeshWithPositionTextureNormalTangentBitangent* Mesh);
	static void fillMesh(Mesh& mesh, MeshStreamingRequest& request);

	template<class ThreadResources>
	void loadMesh(MeshStreamingRequest* request, ThreadResources& tr)
	{
		MeshInfo& meshInfo = meshInfos[request->filename];
		meshInfo.numUsers += 1u;
		if(meshInfo.numUsers == 1u)
		{
			meshInfo.lastRequest = request;
			request->nextMeshRequest = nullptr;

			loadMeshUncached<ThreadResources>(*request);
			return;
		}
		//the resource is loaded or loading
		if(meshInfo.lastRequest == nullptr)
		{
			//The resource is loaded
			request->meshLoaded(*request, &tr, meshInfo.mesh);
			return;
		}
		meshInfo.lastRequest->nextMeshRequest = request;
		meshInfo.lastRequest = request;
		request->nextMeshRequest = nullptr;
	}

	void unloadMesh(UnloadRequest& unloadRequest, void* tr);

	template<class ThreadResources>
	void run(ThreadResources& threadResources)
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
					unloadMesh(static_cast<UnloadRequest&>(message), &threadResources);
				}
				else if(message.meshAction == Action::load)
				{
					loadMesh(static_cast<MeshStreamingRequest*>(&message), threadResources);
				}
				else
				{
					notifyMeshReady(static_cast<MeshStreamingRequest*>(&message), &threadResources);
				}
			}
		} while(!messageQueue.stop());
	}
public:
	MeshManager(AsynchronousFileManager& asynchronousFileManager, StreamingManager& streamingManager, ID3D12Device& graphicsDevice);
	~MeshManager() = default;

	template<class ThreadResources>
	void load(MeshStreamingRequest* request, ThreadResources& threadResources)
	{
		request->meshAction = Action::load;
		addMessage(request, threadResources);
	}

	template<class ThreadResources>
	void unload(Message* request, ThreadResources& threadResources)
	{
		request->meshAction = Action::unload;
		addMessage(request, threadResources);
	}
};