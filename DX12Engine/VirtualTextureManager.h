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
		Message(const wchar_t* filename, void(*deleteRequest)(ReadRequest& request, void* tr))
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
		VirtualTextureManager* virtualTextureManager;
		
		TextureStreamingRequest* nextTextureRequest;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, const Texture& texture);

		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, const Texture& texture),
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
		UnloadRequest(const wchar_t* filename, void(*deleteRequest)(ReadRequest& request, void* tr)) :
			Message(filename, deleteRequest)
		{}
	};
private:
	ActorQueue messageQueue;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
	PageProvider& pageProvider;
	StreamingManager& streamingManager;
	GraphicsEngine& graphicsEngine;
	AsynchronousFileManager& asynchronousFileManager;
private:
	void loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest,
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* tr),
		void callback(PageProvider::AllocateTextureRequest& request, void* tr),
		const DDSFileLoader::DdsHeaderDx12& header);

	void notifyTextureReady(TextureStreamingRequest* request, void* tr);

	template<class ThreadResources>
	static void copyFinished(void* requester, void*)
	{
		TextureStreamingRequest& request = *static_cast<TextureStreamingRequest*>(requester);

		request.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request1, void* tr)
		{
			TextureStreamingRequest& request = *static_cast<TextureStreamingRequest*>(&request1);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);

			request.deleteStreamingRequest = [](StreamingManager::StreamingRequest* request1, void* tr)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				auto& request = *static_cast<TextureStreamingRequest*>(request1);
				VirtualTextureManager& virtualTextureManager = *request.virtualTextureManager;
				
				request.textureAction = Action::notifyReady;
				virtualTextureManager.addMessage(&request, threadResources);
			};
			StreamingManager& streamingManager = request.virtualTextureManager->streamingManager;
			streamingManager.uploadFinished(&request, threadResources);
		};
		request.virtualTextureManager->asynchronousFileManager.discard(request);
	}

	static void textureUseResourceHelper(TextureStreamingRequest& uploadRequest,
		void(*fileLoadedCallback)(AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* buffer));
	static void fileLoadedCallbackHelper(TextureStreamingRequest& uploadRequest, const unsigned char* buffer, StreamingManager::ThreadLocal& streamingManager,
		void(*copyStarted)(void* requester, void* tr));

	template<class ThreadResources>
	static void textureUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* tr)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.pushBackgroundTask({ useSubresourceRequest, [](void* context, ThreadResources&)
		{
			auto& uploadRequest = *static_cast<TextureStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(context));
			textureUseResourceHelper(uploadRequest, [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager&, void* tr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				auto& streamingManager = threadResources.streamingManager;

				fileLoadedCallbackHelper(uploadRequest, buffer, streamingManager, copyFinished<ThreadResources>);
			});
			uploadRequest.virtualTextureManager->asynchronousFileManager.readFile(uploadRequest);
		} });
	}

	template<class ThreadResources>
	void loadTextureUncached(TextureStreamingRequest& uploadRequest)
	{
		uploadRequest.virtualTextureManager = this;
		uploadRequest.file = asynchronousFileManager.openFileForReading(uploadRequest.filename);
		uploadRequest.start = 0u;
		uploadRequest.end = sizeof(DDSFileLoader::DdsHeaderDx12);
		uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager&, void*, const unsigned char* buffer)
		{
			TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
			VirtualTextureManager& virtualTextureManager = *uploadRequest.virtualTextureManager;
			virtualTextureManager.loadTextureUncachedHelper(uploadRequest, textureUseResource<ThreadResources>,
			[](PageProvider::AllocateTextureRequest& request, void*)
			{
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				uploadRequest.virtualTextureManager->asynchronousFileManager.discard(uploadRequest);
			}, *reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer));
		};
		uploadRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request1, void* tr)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			auto& request = static_cast<TextureStreamingRequest&>(request1);
			auto& streamingManager = request.virtualTextureManager->streamingManager;
			streamingManager.addUploadRequest(&request, threadResources);
		};
		asynchronousFileManager.readFile(uploadRequest);
	}

	static D3D12Resource createResource(ID3D12Device& graphicsDevice, const TextureStreamingRequest& request);
	static unsigned int createTextureDescriptor(GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request);
	void createVirtualTexture(ID3D12CommandQueue& commandQueue,
		void callback(PageProvider::AllocateTextureRequest& request, void* tr), TextureStreamingRequest& vramRequest);

	template<class ThreadResources>
	void loadTexture(ThreadResources& executor, TextureStreamingRequest* request)
	{
		const wchar_t * filename = request->filename;

		Texture& texture = textures[filename];
		texture.numUsers += 1u;
		if(texture.numUsers == 1u) //This is the first request, load the resource
		{
			texture.lastRequest = request;
			request->nextTextureRequest = nullptr;

			request->texture = &texture;
			loadTextureUncached<ThreadResources>(*request);
			return;
		}
		if(texture.lastRequest == nullptr)
		{
			//the resource is loaded
			request->textureLoaded(*request, &executor, texture);
			return;
		}
		//the resourse is loading
		texture.lastRequest->nextTextureRequest = request;
		texture.lastRequest = request;
		request->nextTextureRequest = nullptr;
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(UnloadRequest& unloadRequest, void* tr);

	template<class ThreadResources>
	void addMessage(Message* request, ThreadResources& threadResources)
	{
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources)
			{
				auto& manager = *static_cast<VirtualTextureManager*>(requester);
				manager.run(threadResources);
			}});
		}
	}

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
				if(message.textureAction == Action::unload)
				{
					unloadTexture(static_cast<UnloadRequest&>(message), &threadResources);
				}
				else if(message.textureAction == Action::load)
				{
					loadTexture(threadResources, static_cast<TextureStreamingRequest*>(&message));
				}
				else
				{
					notifyTextureReady(static_cast<TextureStreamingRequest*>(&message), &threadResources);
				}
			}
		} while(!messageQueue.stop());
	}
public:
	VirtualTextureManager(PageProvider& pageProvider, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager);
	~VirtualTextureManager() = default;

	template<class ThreadResources>
	void load(TextureStreamingRequest* request, ThreadResources& threadResources)
	{
		request->textureAction = Action::load;
		addMessage(request, threadResources);
	}

	template<class ThreadResources>
	void unload(UnloadRequest* request, ThreadResources& threadResources)
	{
		request->textureAction = Action::unload;
		request->textures = &textures;
		addMessage(request, threadResources);
	}
};