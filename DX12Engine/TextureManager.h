#pragma once

#include "D3D12Resource.h"
#include <unordered_map>
#include <atomic>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "AsynchronousFileManager.h"
#include "IOCompletionQueue.h"
#include "ActorQueue.h"
#include "ResourceLocation.h"
class GraphicsEngine;

class TextureManager
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
		Action textureAction;

		Message() {}
		Message(ResourceLocation filename, void(*deleteRequest)(ReadRequest& request, void* tr))
		{
			start = filename.start;
			end = filename.end;
			this->deleteReadRequest = deleteRequest;
		}
	};

	class TextureStreamingRequest : public StreamingManager::StreamingRequest, public Message
	{
	public:
		ResourceLocation resourceLocation;
		uint32_t width;
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		uint32_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint16_t arraySize;
		unsigned int discriptorIndex;
		ID3D12Resource* resource;
		TextureManager* textureManager;

		TextureStreamingRequest* nextTextureRequest;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor);

		TextureStreamingRequest() {}
		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor),
			ResourceLocation filename) :
			textureLoaded(textureLoaded)
		{
			resourceLocation = filename;
		}
	};

	using UnloadRequest = Message;
private:
	struct Texture
	{
		Texture() : resource(), numUsers(0u), lastRequest(nullptr) {}
		D3D12Resource resource;
		unsigned int descriptorIndex;
		unsigned int numUsers;
		TextureStreamingRequest* lastRequest;
	};

	struct Hash
	{
		std::size_t operator()(ResourceLocation resourceLocation) const noexcept
		{
			return static_cast<std::size_t>(resourceLocation.start * 32u + resourceLocation.end);
		}
	};

	ActorQueue messageQueue;
	std::unordered_map<ResourceLocation, Texture, Hash> textures;
	AsynchronousFileManager& asynchronousFileManager;
	StreamingManager& streamingManager;
	GraphicsEngine& graphicsEngine;

	static ID3D12Resource* createTexture(const TextureStreamingRequest& useSubresourceRequest, GraphicsEngine& graphicsEngine, unsigned int& discriptorIndex);

	void notifyTextureReady(TextureStreamingRequest* request, void* tr);

	template<class ThreadResources>
	static void copyFinished(void* requester, void* tr)
	{
		TextureStreamingRequest& request = *static_cast<TextureStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);

		request.deleteStreamingRequest = [](StreamingManager::StreamingRequest* request1, void* tr)
		{
			auto& threadResources = *static_cast<ThreadResources*>(tr);
			TextureStreamingRequest& request = *static_cast<TextureStreamingRequest*>(request1);
			TextureManager& textureManager = *request.textureManager;
			
			request.textureAction = Action::notifyReady;
			textureManager.addMessage(request, threadResources);
		};
		StreamingManager& streamingManager = request.textureManager->streamingManager;
		streamingManager.uploadFinished(&request, threadResources);
	}
	
	template<class ThreadResources>
	static void textureStreamResource(StreamingManager::StreamingRequest* request, void* tr)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.pushBackgroundTask({request, [](void* requester, ThreadResources&)
		{
			TextureStreamingRequest& uploadRequest = *static_cast<TextureStreamingRequest*>(requester);
			std::size_t resourceSize = DDSFileLoader::resourceSize(uploadRequest.width, uploadRequest.height, uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format);
			constexpr std::size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

			uploadRequest.start = uploadRequest.resourceLocation.start + fileOffset;
			uploadRequest.end = uploadRequest.start + resourceSize;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* buffer)
			{
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				StreamingManager::ThreadLocal& streamingManager = threadResources.streamingManager;
				uploadRequest.resource = createTexture(uploadRequest, uploadRequest.textureManager->graphicsEngine, uploadRequest.discriptorIndex);

				DDSFileLoader::copyResourceToGpu(uploadRequest.resource, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, uploadRequest.width, uploadRequest.height,
					uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format, uploadRequest.uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());

				uploadRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request1, void* tr)
				{
					TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request1);
					ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
					StreamingManager::ThreadLocal& streamingManager = threadResources.streamingManager;

					streamingManager.addCopyCompletionEvent(&uploadRequest, copyFinished<ThreadResources>);
				};
				asynchronousFileManager.discard(uploadRequest);
			};
			uploadRequest.textureManager->asynchronousFileManager.read(uploadRequest);
		} });
	}

	static void loadTextureFromMemory(const unsigned char* buffer, TextureStreamingRequest& uploadRequest,
		void(*streamResource)(StreamingManager::StreamingRequest* request, void* tr));

	template<class ThreadResources>
	void loadTextureUncached(TextureStreamingRequest* request)
	{
		request->textureManager = this;
		request->start = request->resourceLocation.start;
		request->end = request->start + sizeof(DDSFileLoader::DdsHeaderDx12);
		request->fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* buffer)
		{
			TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
			loadTextureFromMemory(buffer, uploadRequest, textureStreamResource<ThreadResources>);
			asynchronousFileManager.discard(request);
		};
		request->deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request1, void* tr)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			auto& request = static_cast<TextureStreamingRequest&>(request1);
			StreamingManager& streamingManager = request.textureManager->streamingManager;
			streamingManager.addUploadRequest(&request, threadResources);
		};
		asynchronousFileManager.read(*request);
	}

	template<class ThreadResources>
	void loadTexture(ThreadResources& threadResources, TextureStreamingRequest* request)
	{
		Texture& texture = textures[request->resourceLocation];
		texture.numUsers += 1u;
		if(texture.numUsers == 1u)
		{
			texture.lastRequest = request;
			request->nextTextureRequest = nullptr;

			loadTextureUncached<ThreadResources>(request);
			return;
		}
		//the resource is loaded or loading
		if(texture.lastRequest == nullptr)
		{
			//The resource is loaded
			request->textureLoaded(*request, &threadResources, texture.descriptorIndex);
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
	void addMessage(Message& request, ThreadResources& threadResources)
	{
		bool needsStarting = messageQueue.push(&request);
		if(needsStarting)
		{
			threadResources.taskShedular.pushBackgroundTask({this, [](void* requester, ThreadResources& threadResources)
			{
				auto& manager = *static_cast<TextureManager*>(requester);
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
	TextureManager(AsynchronousFileManager& asynchronousFileManager, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine);
	~TextureManager() = default;

	template<class ThreadResources>
	void load(TextureStreamingRequest& request, ThreadResources& threadResources)
	{
		request.textureAction = Action::load;
		addMessage(request, threadResources);
	}

	template<class ThreadResources>
	void unload(UnloadRequest& request, ThreadResources& threadResources)
	{
		request.textureAction = Action::unload;
		addMessage(request, threadResources);
	}
};