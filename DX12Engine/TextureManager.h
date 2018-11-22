#pragma once

#include "D3D12Resource.h"
#include <unordered_map>
#include <mutex>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "AsynchronousFileManager.h"
#include "IOCompletionQueue.h"
#include "StreamingRequest.h"
class D3D12GraphicsEngine;

class TextureManager
{
public:
	class TextureStreamingRequest : public StreamingRequest, public AsynchronousFileManager::IORequest
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
			void(*deallocateNode)(StreamingRequest* request, void* threadResources, void* globalResources),
			const wchar_t * filename) :
			textureLoaded(textureLoaded),
			StreamingRequest(deallocateNode)
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
	static void textureStreamResource(StreamingRequest* request, void* tr, void* globalResources)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			TextureStreamingRequest& uploadRequest = *reinterpret_cast<TextureStreamingRequest*>(requester);
			std::size_t resourceSize = DDSFileLoader::resourceSize(uploadRequest.width, uploadRequest.height, uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format);
			constexpr size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);

			uploadRequest.start = fileOffset;
			uploadRequest.end = fileOffset + resourceSize;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
			{
				TextureStreamingRequest& uploadRequest = reinterpret_cast<TextureStreamingRequest&>(request);
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
				StreamingManager::ThreadLocal& streamingManager = threadResources.streamingManager;
				ID3D12Resource* destResource = createTexture(uploadRequest, globalResources.textureManager, globalResources.graphicsEngine, uploadRequest.filename);

				DDSFileLoader::copyResourceToGpu(destResource, uploadRequest.uploadResource, uploadRequest.data.uploadResourceOffset, uploadRequest.width, uploadRequest.height,
					uploadRequest.depth, uploadRequest.mipLevels, uploadRequest.arraySize, uploadRequest.format, uploadRequest.uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());
				streamingManager.copyStarted(threadResources.taskShedular.index(), uploadRequest);
			};
			bool result = globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &uploadRequest);
			assert(result);
		} });
	}

	void textureUploadedHelper(const wchar_t* filename, void* tr, void* gr);

	template<class GlobalResources>
	static void textureUploaded(StreamingRequest* request, void* tr, void* gr)
	{
		const wchar_t* filename = reinterpret_cast<TextureStreamingRequest*>(request)->filename;
		auto& globalResources = *reinterpret_cast<GlobalResources*>(gr);
		auto& textureManager = globalResources.textureManager;

		textureManager.textureUploadedHelper(filename, tr, gr);
	}

	static void loadTextureFromMemory(StreamingManager& streamingManager, const unsigned char* buffer, File file, TextureStreamingRequest& uploadRequest,
		void(*streamResource)(StreamingRequest* request, void* threadResources, void* globalResources), void(*textureUploaded)(StreamingRequest* request, void*, void*));

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(AsynchronousFileManager& asynchronousFileManager, IOCompletionQueue& ioCompletionQueue, ThreadResources& threadResources, GlobalResources& globalResources, TextureStreamingRequest* request)
	{
		request->file = asynchronousFileManager.openFileForReading<GlobalResources>(ioCompletionQueue, request->filename);
		request->start = 0u;
		request->end = sizeof(DDSFileLoader::DdsHeaderDx12);
		request->fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
		{
			GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
			loadTextureFromMemory(globalResources.streamingManager, buffer, request.file, static_cast<TextureStreamingRequest&>(request), textureStreamResource<ThreadResources, GlobalResources>, textureUploaded<GlobalResources>);
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