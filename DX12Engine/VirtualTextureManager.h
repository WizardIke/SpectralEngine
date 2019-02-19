#pragma once
#include "D3D12Resource.h"
#include <unordered_map>
#include <mutex>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "D3D12GraphicsEngine.h"
#include "VirtualTextureInfo.h"
#include "PageProvider.h"
#include "VirtualTextureInfoByID.h"
#include "AsynchronousFileManager.h"
#undef min
#undef max
#include <atomic>

class VirtualTextureManager
{
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

	class TextureStreamingRequest : public StreamingManager::StreamingRequest, public AsynchronousFileManager::ReadRequest
	{
	public:
		constexpr static unsigned int numberOfComponents = 3u; //texture header, texture data, stream to gpu request
		std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		ID3D12Resource* destResource;
		uint32_t width;
		uint32_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint16_t mostDetailedMip;
		TextureStreamingRequest* nextTextureRequest;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture);
		void(*deleteTextureRequest)(TextureStreamingRequest& request);

		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture),
			void(*deleteTextureRequest)(TextureStreamingRequest& request),
			const wchar_t * filename) :
			textureLoaded(textureLoaded),
			deleteTextureRequest(deleteTextureRequest)
		{
			this->filename = filename;
		}
	};
private:
	std::mutex mutex;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
public:
	VirtualTextureInfoByID texturesByID;
	PageProvider pageProvider;
private:
	static void freeRequestMemory(StreamingManager::StreamingRequest* request, void*, void*);

	void notifyTextureReady(TextureStreamingRequest* request, void* executor, void* sharedResources);

	void loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest, StreamingManager& streamingManager, D3D12GraphicsEngine& graphicsEngine,
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
		const DDSFileLoader::DdsHeaderDx12& header);

	template<class ThreadResources, class GlobalResources>
	static void copyFinished(void* requester, void* tr, void* gr)
	{
		TextureStreamingRequest* request = static_cast<TextureStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
		StreamingManager& streamingManager = globalResources.streamingManager;
		streamingManager.uploadFinished(request, threadResources, globalResources);

		auto& virtualTextureManager = globalResources.virtualTextureManager;
		virtualTextureManager.notifyTextureReady(request, tr, gr);
	}

	static void textureUseResourceHelper(TextureStreamingRequest& uploadRequest, void(*fileLoadedCallback)(AsynchronousFileManager::ReadRequest& request, void* tr, void*, const unsigned char* buffer));
	static void fileLoadedCallbackHelper(TextureStreamingRequest& uploadRequest, const unsigned char* buffer, StreamingManager::ThreadLocal& streamingManager,
		void(*copyStarted)(void* requester, void* tr, void* gr));

	template<class ThreadResources, class GlobalResources>
	static void textureUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* executor, void*)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		threadResources.taskShedular.backgroundQueue().push({ useSubresourceRequest, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *static_cast<TextureStreamingRequest*>(static_cast<StreamingManager::StreamingRequest*>(context));
			textureUseResourceHelper(uploadRequest, [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				auto& streamingManager = threadResources.streamingManager;

				fileLoadedCallbackHelper(uploadRequest, buffer, streamingManager, copyFinished<ThreadResources, GlobalResources>);
				globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
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
			auto& virtualTextureManager = globalResources.virtualTextureManager;
			auto& streamingManager = globalResources.streamingManager;
			auto& graphicsEngine = globalResources.graphicsEngine;
			virtualTextureManager.loadTextureUncachedHelper(uploadRequest, streamingManager, graphicsEngine, textureUseResource<ThreadResources, GlobalResources>,
				*reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer));
			globalResources.asynchronousFileManager.discard(&request, threadResources, globalResources);
			streamingManager.addUploadRequest(&uploadRequest, threadResources, globalResources);
		};
		uploadRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr, void* gr)
		{
			freeRequestMemory(static_cast<TextureStreamingRequest*>(&request), tr, gr);
		};
		sharedResources.asynchronousFileManager.readFile(&uploadRequest, executor, sharedResources);
	}

	static D3D12Resource createTexture(ID3D12Device* graphicsDevice, const TextureStreamingRequest& request);
	static unsigned int createTextureDescriptor(D3D12GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request);
	ID3D12Resource* createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue& commandQueue, TextureStreamingRequest& vramRequest);

	void unloadTextureHelper(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, StreamingManager& streamingManager);
public:
	VirtualTextureManager();
	~VirtualTextureManager() {}

	template<class ThreadResources, class GlobalResources>
	static void loadTexture(ThreadResources& executor, GlobalResources& sharedResources, TextureStreamingRequest* request)
	{
		const wchar_t * filename = request->filename;
		auto& virtualTextureManager = sharedResources.virtualTextureManager;

		std::unique_lock<decltype(virtualTextureManager.mutex)> lock(virtualTextureManager.mutex);
		Texture& texture = virtualTextureManager.textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u) //This is the first request, load the resource
		{
			texture.lastRequest = request;
			request->nextTextureRequest = nullptr;
			lock.unlock();

			virtualTextureManager.loadTextureUncached<ThreadResources, GlobalResources>(*request, executor, sharedResources);
			return;
		}
		if (texture.lastRequest == nullptr)
		{
			//the resource is loaded
			lock.unlock();
			request->textureLoaded(*request, &executor, &sharedResources, texture);
			return;
		}
		//the resourse is loading
		texture.lastRequest->nextTextureRequest = request;
		texture.lastRequest = request;
		request->nextTextureRequest = nullptr;
	}

	/*the texture must no longer be in use, including by the GPU*/
	template<class SharedResources_t>
	void unloadTexture(const wchar_t * filename, SharedResources_t& sharedResources)
	{
		unloadTextureHelper(filename, sharedResources.graphicsEngine, sharedResources.streamingManager);
	}
};