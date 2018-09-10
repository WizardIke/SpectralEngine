#pragma once
#include "D3D12Resource.h"
#include <unordered_map>
#include <mutex>
#include "ResizingArray.h"
#include "Delegate.h"
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "D3D12GraphicsEngine.h"
#include "VirtualTextureInfo.h"
#include "PageProvider.h"
#include "VirtualTextureInfoByID.h"
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
	using Request = Delegate<void(void* executor, void* sharedResources, const Texture& texture)>;
private:
	std::mutex mutex;
	std::unordered_map<const wchar_t * const, ResizingArray<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
public:
	VirtualTextureInfoByID texturesByID;
	PageProvider pageProvider;
private:

	template<class GlobalResources>
	static void textureUploaded(void* storedFilename, void* executor, void* sharedResources)
	{
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
		auto& virtualTextureManager = globalResources.virtualTextureManager;

		Texture* texture;
		auto requests = virtualTextureManager.textureUploadedHelper(storedFilename, texture);
		for (auto& request : requests)
		{
			request(executor, sharedResources, *texture);
		}
	}

	ResizingArray<VirtualTextureManager::Request> textureUploadedHelper(void* storedFilename, Texture*& texture);

	void loadTextureUncachedHelper(const wchar_t * filename, File file, StreamingManager& streamingManager, D3D12GraphicsEngine& graphicsEngine,
		void(*useSubresource)(void* executor, void* sharedResources, HalfFinishedUploadRequest& useSubresourceRequest),
		void(*resourceUploaded)(void* requester, void* executor, void* sharedResources),
		const DDSFileLoader::DdsHeaderDx12& header);

	template<class ThreadResources, class GlobalResources>
	static void textureUseSubresource(void* executor, void* sharedResources, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(context);
			auto& uploadRequest = *useSubresourceRequest.uploadRequest;
			const wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);

			size_t subresouceWidth = uploadRequest.textureInfo.width;
			size_t subresourceHeight = uploadRequest.textureInfo.height;
			size_t subresourceDepth = uploadRequest.textureInfo.depth;
			size_t fileOffset = sizeof(DDSFileLoader::DdsHeaderDx12);
			size_t currentMip = 0u;
			while (currentMip != useSubresourceRequest.currentMipLevel)
			{
				size_t numBytes, numRows, rowBytes;
				DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.textureInfo.format, numBytes, rowBytes, numRows);
				fileOffset += numBytes * subresourceDepth;

				subresouceWidth >>= 1u;
				if (subresouceWidth == 0u) subresouceWidth = 1u;
				subresourceHeight >>= 1u;
				if (subresourceHeight == 0u) subresourceHeight = 1u;
				subresourceDepth >>= 1u;
				if (subresourceDepth == 0u) subresourceDepth = 1u;

				++currentMip;
			}

			size_t numBytes, numRows, rowBytes;
			DDSFileLoader::surfaceInfo(subresouceWidth, subresourceHeight, uploadRequest.textureInfo.format, numBytes, rowBytes, numRows);
			size_t subresourceSize = numBytes * subresourceDepth;

			globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, filename, fileOffset, fileOffset + subresourceSize, uploadRequest.file, &useSubresourceRequest,
				[](void* context, void* executor, void* sharedResources, const unsigned char* buffer, File file)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
				HalfFinishedUploadRequest& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(context);
				auto& uploadRequest = *useSubresourceRequest.uploadRequest;
				const wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
				auto& streamingManager = threadResources.streamingManager;
				VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
				ID3D12Resource* destResource;
				{
					std::lock_guard<decltype(virtualTextureManager.mutex)> lock(virtualTextureManager.mutex);
					destResource = virtualTextureManager.textures[filename].resource;
				}

				DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, useSubresourceRequest.uploadResourceOffset, uploadRequest.textureInfo.width, uploadRequest.textureInfo.height,
					uploadRequest.textureInfo.depth, useSubresourceRequest.currentMipLevel, uploadRequest.mipLevels, useSubresourceRequest.currentArrayIndex, uploadRequest.textureInfo.format,
					(unsigned char*)useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos, buffer, &streamingManager.copyCommandList());

				streamingManager.copyStarted(threadResources.taskShedular.index(), useSubresourceRequest);
			});
		} });
	}

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(const wchar_t* filename, ThreadResources& executor, GlobalResources& sharedResources)
	{
		File file = sharedResources.asynchronousFileManager.openFileForReading<GlobalResources>(sharedResources.ioCompletionQueue, filename);

		sharedResources.asynchronousFileManager.readFile(&executor, &sharedResources, filename, 0u, sizeof(DDSFileLoader::DdsHeaderDx12), file, reinterpret_cast<void*>(const_cast<wchar_t *>(filename)),
			[](void* requester, void* executor, void* sharedResources, const unsigned char* buffer, File file)
		{
			const wchar_t* filename = reinterpret_cast<const wchar_t*>(requester);
			ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
			GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
			auto& virtualTextureManager = globalResources.virtualTextureManager;
			auto& streamingManager = globalResources.streamingManager;
			auto& graphicsEngine = globalResources.graphicsEngine;
			virtualTextureManager.loadTextureUncachedHelper(filename, file, streamingManager, graphicsEngine, textureUseSubresource<ThreadResources, GlobalResources>,
				textureUploaded<GlobalResources>, *reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer));
		});
	}

	void createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, RamToVramUploadRequest& vramRequest, const wchar_t* filename, File file);

	void unloadTextureHelper(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, StreamingManager& streamingManager);
public:
	VirtualTextureManager(D3D12GraphicsEngine& graphicsEngine) : pageProvider(graphicsEngine.adapter, graphicsEngine.graphicsDevice) {}
	~VirtualTextureManager() {}

	template<class ThreadResources, class GlobalResources>
	static void loadTexture(ThreadResources& executor, GlobalResources& sharedResources, const wchar_t * filename, Request callback)
	{
		auto& virtualTextureManager = sharedResources.virtualTextureManager;
		auto& graphicsEngine = sharedResources.graphicsEngine;
		auto& streamingManager = executor.streamingManager;

		std::unique_lock<decltype(virtualTextureManager.mutex)> lock(virtualTextureManager.mutex);
		Texture& texture = virtualTextureManager.textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			virtualTextureManager.uploadRequests[filename].push_back(callback);
			lock.unlock();

			virtualTextureManager.loadTextureUncached<ThreadResources, GlobalResources>(filename, executor, sharedResources);
			return;
		}
		if (texture.loaded) //the resource is loaded
		{
			lock.unlock();
			callback(&executor, &sharedResources, texture);
			return;
		}
		//the resourse is loading
		auto& requests = virtualTextureManager.uploadRequests[filename];
		requests.push_back(callback);
	}

	/*the texture must no longer be in use, including by the GPU*/
	template<class SharedResources_t>
	void unloadTexture(const wchar_t * filename, SharedResources_t& sharedResources)
	{
		unloadTextureHelper(filename, sharedResources.graphicsEngine, sharedResources.streamingManager);
	}
};