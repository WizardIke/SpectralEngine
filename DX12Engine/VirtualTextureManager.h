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

class VirtualTextureManager
{
public:
	struct Texture
	{
		Texture() : resource(), numUsers(0u), loaded(false) {}
		ID3D12Resource* resource;
		unsigned int descriptorIndex;
		unsigned int textureID;
		unsigned int numUsers;
		bool loaded;
	};

	class TextureStreamingRequest : public StreamingManager::StreamingRequest, public AsynchronousFileManager::IORequest
	{
	public:
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		ID3D12Resource* destResource;
		uint32_t width;
		uint32_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint16_t mostDetailedMip;

		void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture);
		TextureStreamingRequest* nextTextureRequest;

		TextureStreamingRequest(void(*textureLoaded)(TextureStreamingRequest& request, void* tr, void* gr, const Texture& texture),
			const wchar_t * filename) :
			textureLoaded(textureLoaded)
		{
			this->filename = filename;
		}
	};
private:
	std::mutex mutex;
	std::unordered_map<const wchar_t * const, TextureStreamingRequest*, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
public:
	VirtualTextureInfoByID texturesByID;
	PageProvider pageProvider;
private:
	template<class GlobalResources>
	static void textureUploaded(StreamingManager::StreamingRequest* request, void* executor, void* sharedResources)
	{
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
		auto& virtualTextureManager = globalResources.virtualTextureManager;
		virtualTextureManager.textureUploadedHelper(reinterpret_cast<TextureStreamingRequest*>(request)->filename, executor, sharedResources);
	}

	void textureUploadedHelper(const wchar_t* filename, void* executor, void* sharedResources);

	void loadTextureUncachedHelper(TextureStreamingRequest& uploadRequest, StreamingManager& streamingManager, D3D12GraphicsEngine& graphicsEngine,
		void(*useSubresource)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
		void(*resourceUploaded)(StreamingManager::StreamingRequest* request, void* threadResources, void* globalResources),
		const DDSFileLoader::DdsHeaderDx12& header);

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
	static void textureUseResource(StreamingManager::StreamingRequest* useSubresourceRequest, void* executor, void*)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& uploadRequest = *reinterpret_cast<TextureStreamingRequest*>(context);

			std::size_t subresouceWidth = uploadRequest.width;
			std::size_t subresourceHeight = uploadRequest.height;
			std::size_t subresourceDepth = uploadRequest.depth;
			std::size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);
			
			for (std::size_t currentMip = 0u; currentMip != uploadRequest.mostDetailedMip; ++currentMip)
			{
				std::size_t numBytes, numRows, rowBytes;
				DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.format, numBytes, rowBytes, numRows);
				fileOffset += numBytes * subresourceDepth;

				subresouceWidth >>= 1u;
				if (subresouceWidth == 0u) subresouceWidth = 1u;
				subresourceHeight >>= 1u;
				if (subresourceHeight == 0u) subresourceHeight = 1u;
				subresourceDepth >>= 1u;
				if (subresourceDepth == 0u) subresourceDepth = 1u;
			}

			std::size_t numBytes, numRows, rowBytes;
			DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.format, numBytes, rowBytes, numRows);
			std::size_t subresourceSize = numBytes * subresourceDepth;

			uploadRequest.start = fileOffset;
			uploadRequest.end = fileOffset + subresourceSize;
			uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void*, const unsigned char* buffer)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
				auto& streamingManager = threadResources.streamingManager;
				ID3D12Resource* destResource = uploadRequest.destResource;

				uint32_t subresouceWidth = uploadRequest.width;
				uint32_t subresourceHeight = uploadRequest.height;
				uint32_t subresourceDepth = uploadRequest.depth;
				for(unsigned int currentMip = (unsigned int)uploadRequest.mostDetailedMip; currentMip != (unsigned int)uploadRequest.mipLevels; ++currentMip)
				{
					DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, uploadRequest.uploadResourceOffset, subresouceWidth, subresourceHeight,
						subresourceDepth, (uint32_t)currentMip, uploadRequest.mipLevels, 0u, uploadRequest.format,
						uploadRequest.uploadBufferCurrentCpuAddress, buffer, &streamingManager.copyCommandList());

					subresouceWidth >>= 1u;
					if(subresouceWidth == 0u) subresouceWidth = 1u;
					subresourceHeight >>= 1u;
					if(subresourceHeight == 0u) subresourceHeight = 1u;
					subresourceDepth >>= 1u;
					if(subresourceDepth == 0u) subresourceDepth = 1u;
				}
				streamingManager.addCopyCompletionEvent(&uploadRequest, copyStarted<ThreadResources, GlobalResources>);
			};
			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, &uploadRequest);
		} });
	}

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(TextureStreamingRequest& uploadRequest, ThreadResources& executor, GlobalResources& sharedResources)
	{
		uploadRequest.file = sharedResources.asynchronousFileManager.openFileForReading<GlobalResources>(sharedResources.ioCompletionQueue, uploadRequest.filename);

		uploadRequest.start = 0u;
		uploadRequest.end = sizeof(DDSFileLoader::DdsHeaderDx12);
		uploadRequest.fileLoadedCallback = [](AsynchronousFileManager::IORequest& request, void* tr, void* gr, const unsigned char* buffer)
		{
			TextureStreamingRequest& uploadRequest = static_cast<TextureStreamingRequest&>(request);
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			auto& virtualTextureManager = globalResources.virtualTextureManager;
			auto& streamingManager = globalResources.streamingManager;
			auto& graphicsEngine = globalResources.graphicsEngine;
			virtualTextureManager.loadTextureUncachedHelper(uploadRequest, streamingManager, graphicsEngine, textureUseResource<ThreadResources, GlobalResources>,
				textureUploaded<GlobalResources>, *reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer));
			streamingManager.addUploadRequest(&uploadRequest, threadResources, globalResources);
		};
		sharedResources.asynchronousFileManager.readFile(&executor, &sharedResources, &uploadRequest);
	}

	static D3D12Resource createTexture(ID3D12Device* graphicsDevice, const TextureStreamingRequest& request);
	static unsigned int createTextureDescriptor(D3D12GraphicsEngine& graphicsEngine, ID3D12Resource* texture, const TextureStreamingRequest& request);
	ID3D12Resource* createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue& commandQueue, TextureStreamingRequest& vramRequest);

	void unloadTextureHelper(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, StreamingManager& streamingManager);
public:
	VirtualTextureManager(D3D12GraphicsEngine& graphicsEngine) : pageProvider(graphicsEngine.adapter) {}
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
			auto& requests = virtualTextureManager.uploadRequests[filename];
			request->nextTextureRequest = requests;
			requests = request;
			lock.unlock();

			virtualTextureManager.loadTextureUncached<ThreadResources, GlobalResources>(*request, executor, sharedResources);
			return;
		}
		if (texture.loaded) //the resource is loaded
		{
			lock.unlock();
			request->textureLoaded(*request, &executor, &sharedResources, texture);
			return;
		}
		//the resourse is loading
		auto& requests = virtualTextureManager.uploadRequests[filename];
		request->nextTextureRequest = requests;
		requests = request;
	}

	/*the texture must no longer be in use, including by the GPU*/
	template<class SharedResources_t>
	void unloadTexture(const wchar_t * filename, SharedResources_t& sharedResources)
	{
		unloadTextureHelper(filename, sharedResources.graphicsEngine, sharedResources.streamingManager);
	}
};