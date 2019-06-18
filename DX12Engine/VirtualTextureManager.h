#pragma once
#include "D3D12Resource.h"
#include <unordered_map>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "GraphicsEngine.h"
#include "VirtualTextureInfo.h"
#include "GpuHeapLocation.h"
#include "PageProvider.h"
#include "AsynchronousFileManager.h"
#include "ActorQueue.h"
#include "VirtualTextureInfo.h"
#undef min
#undef max
#include <atomic>

/*
 * Textures must be power of 2 size.
 */
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

	struct Texture : public VirtualTextureInfo
	{
		Texture() : numUsers(0u), lastRequest(nullptr) {}
		unsigned int descriptorIndex;
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

	class TextureStreamingRequest : public StreamingManager::StreamingRequest, public Message, public PageProvider::AllocateTextureRequest
	{
	public:
		Texture* texture;
		D3D12_RESOURCE_DIMENSION dimension;
		AsynchronousFileManager* asynchronousFileManager;
		TextureStreamingRequest* nextTextureRequest;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture);

		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture),
			const wchar_t * filename) :
			textureLoaded(textureLoaded)
		{
			this->filename = filename;
		}
	};

	class UnloadRequest : public Message, public PageProvider::UnloadRequest
	{
		friend class VirtualTextureManager;
		std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>>* textures;
	public:
		UnloadRequest(const wchar_t* filename, void(*deleteRequest)(ReadRequest& request, void* tr, void* gr)) :
			Message(filename, deleteRequest)
		{}
	};
private:
	ActorQueue messageQueue;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
public:
	PageProvider pageProvider;
private:
	void loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine,
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
		void callback(PageProvider::AllocateTextureRequest& request, void* tr, void* gr),
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
		request->asynchronousFileManager->discard(request, threadResources, globalResources);
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
			uploadRequest.asynchronousFileManager->readFile(&uploadRequest, threadResources, globalResources);
		} });
	}

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(TextureStreamingRequest& uploadRequest, ThreadResources& executor, GlobalResources& sharedResources)
	{
		uploadRequest.asynchronousFileManager = &sharedResources.asynchronousFileManager;
		uploadRequest.file = uploadRequest.asynchronousFileManager->openFileForReading<GlobalResources>(sharedResources.ioCompletionQueue, uploadRequest.filename);

		uploadRequest.start = 0u;
		uploadRequest.end = sizeof(DDSFileLoader::DdsHeaderDx12);
		uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void*, void* gr, const unsigned char* buffer)
		{
			TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
			auto& streamingManager = globalResources.streamingManager;
			auto& graphicsEngine = globalResources.graphicsEngine;
			virtualTextureManager.loadTextureUncachedHelper(uploadRequest, streamingManager, graphicsEngine, textureUseResource<ThreadResources, GlobalResources>,
			[](PageProvider::AllocateTextureRequest& request, void* tr, void* gr)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				uploadRequest.asynchronousFileManager->discard(&uploadRequest, threadResources, globalResources);
			}, *reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer));
		};
		uploadRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			auto& streamingManager = globalResources.streamingManager;
			streamingManager.addUploadRequest(static_cast<TextureStreamingRequest*>(&request), threadResources, globalResources);
		};
		uploadRequest.asynchronousFileManager->readFile(&uploadRequest, executor, sharedResources);
	}

	static D3D12Resource createResource(ID3D12Device* graphicsDevice, const TextureStreamingRequest& request);
	static unsigned int createTextureDescriptor(GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request);
	void createVirtualTexture(GraphicsEngine& graphicsEngine, ID3D12CommandQueue& commandQueue,
		void callback(PageProvider::AllocateTextureRequest& request, void* tr, void* gr), TextureStreamingRequest& vramRequest);

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

			request->texture = &texture;
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
	void unloadTexture(UnloadRequest& unloadRequest, GraphicsEngine& graphicsEngine, void* tr, void* gr);

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
					unloadTexture(static_cast<UnloadRequest&>(message), globalResources.graphicsEngine, &threadResources, &globalResources);
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
	void unload(UnloadRequest* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->textureAction = Action::unload;
		request->textures = &textures;
		addMessage(request, threadResources, globalResources);
	}
};