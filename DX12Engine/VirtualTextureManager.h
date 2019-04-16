#pragma once
#include "D3D12Resource.h"
#include <unordered_map>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "GraphicsEngine.h"
#include "VirtualTextureInfo.h"
#include "PageProvider.h"
#include "VirtualTextureInfoByID.h"
#include "AsynchronousFileManager.h"
#include "ActorQueue.h"
#undef min
#undef max
#include <atomic>

class VirtualTextureManager
{
	enum class Action : short
	{
		load,
		unload,
		notifyReady,
	};
public:
	class TextureStreamingRequest;

	struct Texture
	{
		Texture() : numUsers(0u), lastRequest(nullptr) {}
		ID3D12Resource* resource;
		unsigned int descriptorIndex;
		unsigned int textureID;
		unsigned int numUsers;
		TextureStreamingRequest* lastRequest;
	};

	class Message : public AsynchronousFileManager::ReadRequest
	{
	public:
		Action textureAction;

		Message() {}
		Message(const wchar_t* filename, void(*deleteRequest)(ReadRequest& request, void* tr, void* gr))
		{
			this->filename = filename;
			this->deleteReadRequest = deleteRequest;
		}
	};

	class TextureStreamingRequest : public StreamingManager::StreamingRequest, public Message
	{
	public:
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		ID3D12Resource* resource;
		uint32_t width;
		uint32_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint16_t lowestPinnedMip;
		unsigned int widthInPages;
		unsigned int heightInPages;
		unsigned int descriptorIndex;
		HeapLocation* pinnedHeapLocations;
		TextureStreamingRequest* nextTextureRequest;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture);

		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture),
			const wchar_t * filename) :
			textureLoaded(textureLoaded)
		{
			this->filename = filename;
		}
	};
private:
	ActorQueue messageQueue;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
public:
	VirtualTextureInfoByID texturesByID;
	PageProvider pageProvider;
private:
	void loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine,
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
		const DDSFileLoader::DdsHeaderDx12& header);

	void notifyTextureReady(TextureStreamingRequest* request, void* tr, void* gr);

	template<class ThreadResources, class GlobalResources>
	static void copyFinished(void* requester, void* tr, void* gr)
	{
		TextureStreamingRequest* request = static_cast<TextureStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

		request->deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request1, void* tr, void* gr)
		{
			TextureStreamingRequest* request = static_cast<TextureStreamingRequest*>(&request1);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

			request->deleteStreamingRequest = [](StreamingManager::StreamingRequest* request1, void* tr, void* gr)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
				auto request = static_cast<TextureStreamingRequest*>(request1);
				request->textureAction = Action::notifyReady;
				virtualTextureManager.addMessage(request, threadResources, globalResources);
			};
			StreamingManager& streamingManager = globalResources.streamingManager;
			streamingManager.uploadFinished(request, threadResources, globalResources);
		};
		globalResources.asynchronousFileManager.discard(request, threadResources, globalResources);
	}

	static void textureUseResourceHelper(TextureStreamingRequest& uploadRequest, void(*fileLoadedCallback)(AsynchronousFileManager::ReadRequest& request, void* tr, void*, const unsigned char* buffer));
	static void fileLoadedCallbackHelper(TextureStreamingRequest& uploadRequest, const unsigned char* buffer, StreamingManager::ThreadLocal& streamingManager,
		void(*copyStarted)(void* requester, void* tr, void* gr));

	template<class ThreadResources, class GlobalResources>
	static void textureUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* executor, void*)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		threadResources.taskShedular.pushBackgroundTask({ useSubresourceRequest, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *static_cast<TextureStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(context));
			textureUseResourceHelper(uploadRequest, [](AsynchronousFileManager::ReadRequest& request, void* tr, void*, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				auto& streamingManager = threadResources.streamingManager;

				fileLoadedCallbackHelper(uploadRequest, buffer, streamingManager, copyFinished<ThreadResources, GlobalResources>);
			});
			globalResources.asynchronousFileManager.readFile(&uploadRequest, threadResources, globalResources);
		} });
	}

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(TextureStreamingRequest& uploadRequest, ThreadResources& executor, GlobalResources& sharedResources)
	{
		uploadRequest.file = sharedResources.asynchronousFileManager.openFileForReading<GlobalResources>(sharedResources.ioCompletionQueue, uploadRequest.filename);

		uploadRequest.start = 0u;
		uploadRequest.end = sizeof(DDSFileLoader::DdsHeaderDx12);
		uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* buffer)
		{
			TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
			auto& streamingManager = globalResources.streamingManager;
			auto& graphicsEngine = globalResources.graphicsEngine;
			virtualTextureManager.loadTextureUncachedHelper(uploadRequest, streamingManager, graphicsEngine, textureUseResource<ThreadResources, GlobalResources>,
				*reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer));
			globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
		};
		uploadRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			auto& streamingManager = globalResources.streamingManager;
			streamingManager.addUploadRequest(static_cast<TextureStreamingRequest*>(&request), threadResources, globalResources);
		};
		sharedResources.asynchronousFileManager.readFile(&uploadRequest, executor, sharedResources);
	}

	static D3D12Resource createTexture(ID3D12Device* graphicsDevice, const TextureStreamingRequest& request);
	static unsigned int createTextureDescriptor(GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request);
	void createTextureWithResitencyInfo(GraphicsEngine& graphicsEngine, ID3D12CommandQueue& commandQueue, TextureStreamingRequest& vramRequest);

	template<class ThreadResources, class GlobalResources>
	void loadTexture(ThreadResources& executor, GlobalResources& sharedResources, TextureStreamingRequest* request)
	{
		const wchar_t * filename = request->filename;

		Texture& texture = textures[filename];
		texture.numUsers += 1u;
		if(texture.numUsers == 1u) //This is the first request, load the resource
		{
			texture.lastRequest = request;
			request->nextTextureRequest = nullptr;

			loadTextureUncached<ThreadResources, GlobalResources>(*request, executor, sharedResources);
			return;
		}
		if(texture.lastRequest == nullptr)
		{
			//the resource is loaded
			request->textureLoaded(*request, &executor, &sharedResources, texture);
			return;
		}
		//the resourse is loading
		texture.lastRequest->nextTextureRequest = request;
		texture.lastRequest = request;
		request->nextTextureRequest = nullptr;
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t* filename, GraphicsEngine& graphicsEngine, StreamingManager& streamingManager);

	template<class ThreadResources, class GlobalResources>
	void addMessage(Message* request, ThreadResources& threadResources, GlobalResources&)
	{
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& manager = *static_cast<VirtualTextureManager*>(requester);
				manager.run(threadResources, globalResources);
			}});
		}
	}

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
				if(message.textureAction == Action::unload)
				{
					unloadTexture(message.filename, globalResources.graphicsEngine, globalResources.streamingManager);
				}
				else if(message.textureAction == Action::load)
				{
					loadTexture(threadResources, globalResources, static_cast<TextureStreamingRequest*>(&message));
				}
				else
				{
					notifyTextureReady(static_cast<TextureStreamingRequest*>(&message), &threadResources, &globalResources);
				}
			}
		} while(!messageQueue.stop());
	}
public:
	VirtualTextureManager();
	~VirtualTextureManager() {}

	template<class ThreadResources, class GlobalResources>
	void load(TextureStreamingRequest* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->textureAction = Action::load;
		addMessage(request, threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources>
	void unload(Message* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->textureAction = Action::unload;
		addMessage(request, threadResources, globalResources);
	}
};