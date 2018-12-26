#pragma once

#include "D3D12Resource.h"
#include <unordered_map>
#include <mutex>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "AsynchronousFileManager.h"
#include "IOCompletionQueue.h"
class D3D12GraphicsEngine;

class TextureManager
{
public:
	class TextureStreamingRequest : public StreamingManager::StreamingRequest, public AsynchronousFileManager::IORequest
	{
	public:
		uint32_t width;
		DXGI_FORMAT format;
		uint32_t height;
		uint16_t depth;
		uint16_t mipLevels;
		D3D12_RESOURCE_DIMENSION dimension;
		uint16_t arraySize;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureDescriptor);
		TextureStreamingRequest* nextTextureRequest;

		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureDescriptor),
			const wchar_t * filename) :
			textureLoaded(textureLoaded)
		{
			this->filename = filename;
		}
	};
private:
	struct Texture
	{
		Texture() : resource(), numUsers(0u), loaded(false) {}
		D3D12Resource resource;
		unsigned int descriptorIndex;
		unsigned short numUsers;
		bool loaded;
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, TextureStreamingRequest*, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;

	static ID3D12Resource* createTexture(const TextureStreamingRequest& useSubresourceRequest, TextureManager& textureManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t* filename);

	template<class ThreadResources, class GlobalResources>
	static void copyStarted(void* requester, void* tr, void* gr)
	{
		TextureStreamingRequest* request = static_cast<TextureStreamingRequest*>(requester);
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
		StreamingManager& streamingManager = globalResources.streamingManager;
		streamingManager.uploadFinished(request, threadResources, globalResources);
	}
	
	template<class ThreadResources, class GlobalResources>
	static void textureStreamResource(StreamingManager::StreamingRequest* request, void* tr, void*)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({request, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			TextureStreamingRequest& uploadRequest = *static_cast<TextureStreamingRequest*>(requester);
			std::size_t resourceSize = DDSFileLoader::resourceSize(uploadRequest.width, uploadRequest.height, uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format);
			constexpr size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

			uploadRequest.start = fileOffset;
			uploadRequest.end = fileOffset + resourceSize;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				StreamingManager::ThreadLocal& streamingManager = threadResources.streamingManager;
				ID3D12Resource* destResource = createTexture(uploadRequest, globalResources.textureManager, globalResources.graphicsEngine, uploadRequest.filename);

				DDSFileLoader::copyResourceToGpu(destResource, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, uploadRequest.width, uploadRequest.height,
					uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format, uploadRequest.uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());
				streamingManager.addCopyCompletionEvent(&uploadRequest, copyStarted<ThreadResources, GlobalResources>);
			};
			bool result = globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &uploadRequest);
#ifndef ndebug
			if(!result)
				assert(result);
#endif // !ndebug
		} });
	}

	void textureUploadedHelper(const wchar_t* filename, void* tr, void* gr);

	template<class GlobalResources>
	static void textureUploaded(StreamingManager::StreamingRequest* request, void* tr, void* gr)
	{
		const wchar_t* filename = static_cast<TextureStreamingRequest*>(request)->filename;
		auto& globalResources = *static_cast<GlobalResources*>(gr);
		auto& textureManager = globalResources.textureManager;

		textureManager.textureUploadedHelper(filename, tr, gr);
	}

	static void loadTextureFromMemory(const unsigned char* buffer, TextureStreamingRequest& uploadRequest,
		void(*streamResource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources), void(*textureUploaded)(StreamingManager::StreamingRequest* request, void*, void*));

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(AsynchronousFileManager& asynchronousFileManager, IOCompletionQueue& ioCompletionQueue, ThreadResources& threadResources, GlobalResources& globalResources, TextureStreamingRequest* request)
	{
		request->file = asynchronousFileManager.openFileForReading<GlobalResources>(ioCompletionQueue, request->filename);
		request->start = 0u;
		request->end = sizeof(DDSFileLoader::DdsHeaderDx12);
		request->fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
			loadTextureFromMemory(buffer, uploadRequest, textureStreamResource<ThreadResources, GlobalResources>, textureUploaded<GlobalResources>);
			StreamingManager& streamingManager = globalResources.streamingManager;
			streamingManager.addUploadRequest(&uploadRequest, threadResources, globalResources);
		};
		asynchronousFileManager.readFile(&threadResources, &globalResources, request);
	}
public:
	TextureManager();
	~TextureManager();

	template<class ThreadResources, class GlobalResources>
	void loadTexture(ThreadResources& executor, GlobalResources& sharedResources, TextureStreamingRequest* request)
	{
		const wchar_t * filename = request->filename;
		std::unique_lock<decltype(mutex)> lock(mutex);
		Texture& texture = textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			auto& requests = uploadRequests[filename];
			request->nextTextureRequest = requests;
			requests = request;
			lock.unlock();

			loadTextureUncached(sharedResources.asynchronousFileManager, sharedResources.ioCompletionQueue, executor, sharedResources, request);
			return;
		}
		//the resource is loaded or loading
		if (texture.loaded)
		{
			unsigned int textureIndex = texture.descriptorIndex;
			lock.unlock();
			request->textureLoaded(*request, &executor, &sharedResources, textureIndex);
			return;
		}

		auto& requests = uploadRequests[filename];
		request->nextTextureRequest = requests;
		requests = request;
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t* filename, D3D12GraphicsEngine& graphicsEngine);
};