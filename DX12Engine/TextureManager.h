#pragma once

#include "ResizingArray.h"
#include "D3D12Resource.h"
#include "RamToVramUploadRequest.h"
#include <unordered_map>
#include <mutex>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "Delegate.h"
#include "AsynchronousFileManager.h"
#include "IOCompletionQueue.h"
class D3D12GraphicsEngine;

class TextureManager
{
private:
	struct Texture
	{
		Texture() : resource(), numUsers(0u), loaded(false) {}
		D3D12Resource resource;
		unsigned int descriptorIndex;
		unsigned int numUsers;
		bool loaded; //remove this bool
	};
	using Request = Delegate<void(void* executor, void* sharedResources, unsigned int textureDescriptorIndex)>;

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, ResizingArray<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;

	static ID3D12Resource* createOrLookupTexture(const HalfFinishedUploadRequest& useSubresourceRequest, TextureManager& textureManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t* filename);
	
	template<class ThreadResources, class GlobalResources>
	static void textureUseSubresource(void* executor, void* sharedResources, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
		threadResources.taskShedular.backgroundQueue().push({ &useSubresourceRequest, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			HalfFinishedUploadRequest& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
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

			bool result = globalResources.asynchronousFileManager.readFile(&threadResources, &globalResources, filename, fileOffset, fileOffset + subresourceSize, uploadRequest.file, &useSubresourceRequest,
				[](void* requester, void* executor, void* sharedResources, const uint8_t* buffer, File file)
			{
				HalfFinishedUploadRequest& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
				auto& uploadRequest = *useSubresourceRequest.uploadRequest;
				const wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(executor);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
				StreamingManager::ThreadLocal& streamingManager = threadResources.streamingManager;

				ID3D12Resource* destResource = createOrLookupTexture(useSubresourceRequest, globalResources.textureManager, globalResources.graphicsEngine, filename);
				DDSFileLoader::copySubresourceToGpu(destResource, uploadRequest.uploadResource, useSubresourceRequest.uploadResourceOffset, uploadRequest.textureInfo.width, uploadRequest.textureInfo.height,
					uploadRequest.textureInfo.depth, useSubresourceRequest.currentMipLevel, uploadRequest.mipLevels, useSubresourceRequest.currentArrayIndex, uploadRequest.textureInfo.format,
					(uint8_t*)useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos, buffer, &streamingManager.copyCommandList());
				streamingManager.copyStarted(threadResources.taskShedular.index(), useSubresourceRequest);
			});
			assert(result);
		} });
	}

	template<class GlobalResources>
	static void textureUploaded(void* storedFilename, void* executor, void* gr)
	{
		const wchar_t* filename = reinterpret_cast<wchar_t*>(storedFilename);
		auto& globalResources = *reinterpret_cast<GlobalResources*>(gr);
		auto& textureManager = globalResources.textureManager;

		unsigned int descriptorIndex;
		ResizingArray<Request> requests;
		{
			std::lock_guard<decltype(textureManager.mutex)> lock(textureManager.mutex);
			auto& texture = textureManager.textures[filename];
			texture.loaded = true;
			descriptorIndex = texture.descriptorIndex;

			auto requestsPos = textureManager.uploadRequests.find(filename);
			assert(requestsPos != textureManager.uploadRequests.end() && "A texture is loading with no requests for it");
			requests = std::move(requestsPos->second);
			textureManager.uploadRequests.erase(requestsPos);
		}

		for (auto& request : requests)
		{
			request(executor, gr, descriptorIndex);
		}
	}

	static void loadTextureFromMemory(StreamingManager& streamingManager, const uint8_t* buffer, File file, void* requester, void (*textureUseSubresource)(void*, void*, HalfFinishedUploadRequest&), 
		void (*textureUploaded)(void*, void*, void*))
	{
		const DDSFileLoader::DdsHeaderDx12& header = *reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer);
		bool valid = DDSFileLoader::validateDdsHeader(header);
		if (!valid) throw false;
		RamToVramUploadRequest uploadRequest;
		uploadRequest.file = file;
		uploadRequest.useSubresource = textureUseSubresource;
		uploadRequest.requester = requester; //Actually the filename
		uploadRequest.resourceUploadedPointer = textureUploaded;
		uploadRequest.textureInfo.width = header.width;
		uploadRequest.textureInfo.height = header.height;
		uploadRequest.textureInfo.format = header.dxgiFormat;
		uploadRequest.mipLevels = header.mipMapCount;
		uploadRequest.currentArrayIndex = 0u;
		uploadRequest.currentMipLevel = 0u;
		uploadRequest.mostDetailedMip = 0u;
		uploadRequest.dimension = (D3D12_RESOURCE_DIMENSION)header.dimension;
		if (header.miscFlag & 0x4L)
		{
			uploadRequest.arraySize = header.arraySize * 6u; //The texture is a cubemap
		}
		else
		{
			uploadRequest.arraySize = header.arraySize;
		}
		uploadRequest.textureInfo.depth = header.depth;

		streamingManager.addUploadRequest(uploadRequest);
	}

	template<class ThreadResources, class GlobalResources>
	void loadTextureUncached(AsynchronousFileManager& asynchronousFileManager, IOCompletionQueue& ioCompletionQueue, ThreadResources& threadResources, GlobalResources& globalResources, const wchar_t * filename)
	{
		File file = asynchronousFileManager.openFileForReading<GlobalResources>(ioCompletionQueue, filename);

		asynchronousFileManager.readFile(nullptr, &globalResources, filename, 0u, sizeof(DDSFileLoader::DdsHeaderDx12), file, reinterpret_cast<void*>(const_cast<wchar_t *>(filename)),
			[](void* requester, void* tr, void* gr, const uint8_t* buffer, File file)
		{
			GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
			loadTextureFromMemory(globalResources.streamingManager, buffer, file, requester, textureUseSubresource<ThreadResources, GlobalResources>, textureUploaded<GlobalResources>);
		});
	}
public:
	TextureManager();
	~TextureManager();

	template<class ThreadResources, class GlobalResources>
	void loadTexture(ThreadResources& executor, GlobalResources& sharedResources, const wchar_t * filename, Request callback)
	{
		std::unique_lock<decltype(mutex)> lock(mutex);
		Texture& texture = textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			uploadRequests[filename].push_back(callback);
			lock.unlock();

			loadTextureUncached(sharedResources.asynchronousFileManager, sharedResources.taskShedular.ioCompletionQueue(), executor, sharedResources, filename);
			return;
		}
		//the resource is loaded or loading
		if (texture.loaded)
		{
			unsigned int textureIndex = texture.descriptorIndex;
			lock.unlock();
			callback(&executor, &sharedResources, textureIndex);
			return;
		}

		uploadRequests[filename].push_back(callback);
		return;
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine);
};