#pragma once

#include "D3D12Resource.h"
#include <unordered_map>
#include <atomic>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "AsynchronousFileManager.h"
#include "IOCompletionQueue.h"
#include "ActorQueue.h"
class D3D12GraphicsEngine;

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
	};

	class TextureStreamingRequest : public StreamingManager::StreamingRequest, public Message
	{
	public:
		constexpr static unsigned int numberOfComponents = 3u; //texture header, texture data, stream to gpu request
		std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;
		uint32_t width;
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		uint32_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint16_t arraySize;
		unsigned int discriptorIndex;
		ID3D12Resource* resource;
		TextureStreamingRequest* nextTextureRequest;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureDescriptor);
		void(*deleteTextureRequest)(TextureStreamingRequest& request);

		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureDescriptor),
			void(*deleteTextureRequest)(TextureStreamingRequest& request),
			const wchar_t * filename) :
			textureLoaded(textureLoaded),
			deleteTextureRequest(deleteTextureRequest)
		{
			this->filename = filename;
		}
	};
private:
	struct Texture
	{
		Texture() : resource(), numUsers(0u), lastRequest(nullptr) {}
		D3D12Resource resource;
		unsigned int descriptorIndex;
		unsigned int numUsers;
		TextureStreamingRequest* lastRequest;
	};

	ActorQueue messageQueue;
	std::unordered_map<const wchar_t* const, Texture, std::hash<const wchar_t *>> textures;

	static ID3D12Resource* createTexture(const TextureStreamingRequest& useSubresourceRequest, D3D12GraphicsEngine& graphicsEngine, unsigned int& discriptorIndex, const wchar_t* filename);
	void notifyTextureReadyHelper(TextureStreamingRequest* request, void* tr, void* gr);

	template<class ThreadResources, class GlobalResources>
	void notifyTextureReady(TextureStreamingRequest* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		notifyTextureReadyHelper(request, &threadResources, &globalResources);
		StreamingManager& streamingManager = globalResources.streamingManager;
		streamingManager.uploadFinished(request, threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources>
	static void copyFinished(void* requester, void* tr, void* gr)
	{
		TextureStreamingRequest* request = static_cast<TextureStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
		
		TextureManager& textureManager = globalResources.textureManager;
		request->textureAction = Action::notifyReady;
		textureManager.addMessage(static_cast<Message*>(request), threadResources, globalResources);
	}

	static void freeRequestMemory(StreamingManager::StreamingRequest* request, void*, void*);
	
	template<class ThreadResources, class GlobalResources>
	static void textureStreamResource(StreamingManager::StreamingRequest* request, void* tr, void*)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({request, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			TextureStreamingRequest& uploadRequest = *static_cast<TextureStreamingRequest*>(requester);
			std::size_t resourceSize = DDSFileLoader::resourceSize(uploadRequest.width, uploadRequest.height, uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format);
			constexpr std::size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

			uploadRequest.start = fileOffset;
			uploadRequest.end = fileOffset + resourceSize;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				StreamingManager::ThreadLocal& streamingManager = threadResources.streamingManager;
				uploadRequest.resource = createTexture(uploadRequest, globalResources.graphicsEngine, uploadRequest.discriptorIndex, uploadRequest.filename);

				DDSFileLoader::copyResourceToGpu(uploadRequest.resource, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, uploadRequest.width, uploadRequest.height,
					uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format, uploadRequest.uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());
				globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
				streamingManager.addCopyCompletionEvent(&uploadRequest, copyFinished<ThreadResources, GlobalResources>);
			};
			globalResources.asynchronousFileManager.readFile(&uploadRequest, threadResources, globalResources);
		} });
	}

	static void loadTextureFromMemory(const unsigned char* buffer, TextureStreamingRequest& uploadRequest,
		void(*streamResource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources));

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(AsynchronousFileManager& asynchronousFileManager, IOCompletionQueue& ioCompletionQueue, ThreadResources& threadResources, GlobalResources& globalResources, TextureStreamingRequest* request)
	{
		request->file = asynchronousFileManager.openFileForReading<GlobalResources>(ioCompletionQueue, request->filename);
		request->start = 0u;
		request->end = sizeof(DDSFileLoader::DdsHeaderDx12);
		request->fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* buffer)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
			loadTextureFromMemory(buffer, uploadRequest, textureStreamResource<ThreadResources, GlobalResources>);
			globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
			StreamingManager& streamingManager = globalResources.streamingManager;
			streamingManager.addUploadRequest(&uploadRequest, threadResources, globalResources);
		};
		request->deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			freeRequestMemory(&static_cast<TextureStreamingRequest&>(request), tr, gr);
		};
		asynchronousFileManager.readFile(request, threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources>
	void loadTexture(ThreadResources& executor, GlobalResources& sharedResources, TextureStreamingRequest* request)
	{
		Texture& texture = textures[request->filename];
		texture.numUsers += 1u;
		if(texture.numUsers == 1u)
		{
			texture.lastRequest = request;
			request->nextTextureRequest = nullptr;

			loadTextureUncached(sharedResources.asynchronousFileManager, sharedResources.ioCompletionQueue, executor, sharedResources, request);
			return;
		}
		//the resource is loaded or loading
		if(texture.lastRequest == nullptr)
		{
			//The resource is loaded
			request->textureLoaded(*request, &executor, &sharedResources, texture.descriptorIndex);
			return;
		}
		//the resourse is loading
		texture.lastRequest->nextTextureRequest = request;
		texture.lastRequest = request;
		request->nextTextureRequest = nullptr;
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t* filename, D3D12GraphicsEngine& graphicsEngine);

	template<class ThreadResources, class GlobalResources>
	void addMessage(SinglyLinked* request, ThreadResources& threadResources, GlobalResources&)
	{
		bool needsStarting = messageQueue.push(request);
		if(needsStarting)
		{
			threadResources.taskShedular.backgroundQueue().push({this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& manager = *static_cast<TextureManager*>(requester);
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
					unloadTexture(message.filename, globalResources.graphicsEngine);
				}
				else if(message.textureAction == Action::load)
				{
					loadTexture(threadResources, globalResources, static_cast<TextureStreamingRequest*>(&message));
				}
				else
				{
					notifyTextureReady(static_cast<TextureStreamingRequest*>(&message), threadResources, globalResources);
				}
			}
		} while(!messageQueue.stop());
	}
public:
	TextureManager();
	~TextureManager();

	template<class ThreadResources, class GlobalResources>
	void load(TextureStreamingRequest* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->textureAction = Action::load;
		addMessage(static_cast<Message*>(request), threadResources, globalResources);
	}

	template<class ThreadResources, class GlobalResources>
	void unload(Message* request, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		request->textureAction = Action::unload;
		addMessage(request, threadResources, globalResources);
	}
};