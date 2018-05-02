#pragma once
class BaseExecutor;
#include "D3D12Resource.h"
#include <unordered_map>
#include <mutex>
#include "ResizingArray.h"
#include "Job.h"
#include "StreamingManager.h"
#include "DDSFileLoader.h"
#include "D3D12GraphicsEngine.h"
#include "TextureResitency.h"
#include "FixedSizeAllocator.h"
#include "PageProvider.h"
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
	class Request
	{
		friend class VirtualTextureManager;
		void* requester;
		void(*job)(void*const requester, BaseExecutor* const executor, SharedResources& sharedResources, const Texture& texture);
	public:
		Request(void* const requester, void(*job)(void*const requester, BaseExecutor* const executor, SharedResources& sharedResources, const Texture& texture)) :
			requester(requester), job(job) {}
		Request() {}

		void operator()(BaseExecutor* const executor, SharedResources& sharedResources, const Texture& texture)
		{
			job(requester, executor, sharedResources, texture);
		}
	};
private:
	class TextureInfoAllocator
	{
		union Element
		{
			std::aligned_storage_t<sizeof(VirtualTextureInfo), alignof(VirtualTextureInfo)> data;
			Element* next;
		};
		Element mData[255]; //index 255 is reserved for invalid texture ids
		Element* freeList;
	public:
		TextureInfoAllocator()
		{
			Element* newFreeList = nullptr;
			for (auto i = 255u; i != 0u;)
			{
				--i;
				mData[i].next = newFreeList;
				newFreeList = &mData[i];
			}
			freeList = newFreeList;
		}

		~TextureInfoAllocator() 
		{
#ifndef NDEBUG
			unsigned int freeListLength = 0u;
			for (auto i = freeList; i != nullptr; i = i->next)
			{
				++freeListLength;
			}
			assert(freeListLength == 255u && "cannot destruct a TextureInfoAllocator while it is still in use");
#endif
		}

		unsigned int allocate()
		{
			Element* element = freeList;
			freeList = freeList->next;
			return (unsigned int)(element - mData);
		}

		void deallocate(unsigned int index)
		{
			mData[index].next = freeList;
			freeList = &mData[index];
		}

		VirtualTextureInfo& operator[](size_t index)
		{
			return reinterpret_cast<VirtualTextureInfo&>(mData[index].data);
		}

		const VirtualTextureInfo& operator[](size_t index) const
		{
			return reinterpret_cast<const VirtualTextureInfo&>(mData[index].data);
		}
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, ResizingArray<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
public:
	TextureInfoAllocator texturesByID;
	PageProvider pageProvider;
private:

	template<class SharedResources_t>
	static void textureUploaded(void* storedFilename, BaseExecutor* exe, SharedResources& sr)
	{
		auto& sharedResources = reinterpret_cast<SharedResources_t&>(sr);
		auto& virtualTextureManager = sharedResources.virtualTextureManager;

		virtualTextureManager.textureUploadedHelper(storedFilename, exe, sharedResources);
	}

	void textureUploadedHelper(void* storedFilename, BaseExecutor* executor, SharedResources& sharedResources);

	void loadTextureUncachedHelper(const wchar_t * filename, File file, BaseExecutor* executor, SharedResources& sharedResources,
		void(*useSubresource)(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest),
		void(*resourceUploaded)(void* const requester, BaseExecutor* const executor, SharedResources& sharedResources),
		const DDSFileLoader::DdsHeaderDx12& header);

	template<class SharedResources_t>
	static void textureUseSubresource(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		sharedResources.backgroundQueue.push(Job(&useSubresourceRequest, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			auto& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
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

			sharedResources.asynchronousFileManager.readFile(executor, sharedResources, filename, fileOffset, fileOffset + subresourceSize, uploadRequest.file, &useSubresourceRequest,
				[](void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* buffer, File file)
			{
				HalfFinishedUploadRequest& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
				auto& uploadRequest = *useSubresourceRequest.uploadRequest;
				const wchar_t* filename = reinterpret_cast<wchar_t*>(uploadRequest.requester);
				auto& streamingManager = executor->streamingManager;
				VirtualTextureManager& virtualTextureManager = reinterpret_cast<SharedResources_t&>(sharedResources).virtualTextureManager;
				ID3D12Resource* destResource;
				{
					std::lock_guard<decltype(virtualTextureManager.mutex)> lock(virtualTextureManager.mutex);
					destResource = virtualTextureManager.textures[filename].resource;
				}

				DDSFileLoader::copySubresourceToGpuTiled(destResource, uploadRequest.uploadResource, useSubresourceRequest.uploadResourceOffset, uploadRequest.textureInfo.width, uploadRequest.textureInfo.height,
					uploadRequest.textureInfo.depth, useSubresourceRequest.currentMipLevel, uploadRequest.mipLevels, useSubresourceRequest.currentArrayIndex, uploadRequest.textureInfo.format,
					(uint8_t*)useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos, buffer, streamingManager.copyCommandList());

				streamingManager.copyStarted(executor, useSubresourceRequest);
			});
		}));
	}

	template<class SharedResources_t>
	void loadTextureUncached(const wchar_t* filename, BaseExecutor* executor, SharedResources& sharedResources)
	{
		File file = sharedResources.asynchronousFileManager.openFileForReading(sharedResources.ioCompletionQueue, filename);

		sharedResources.asynchronousFileManager.readFile(executor, sharedResources, filename, 0u, sizeof(DDSFileLoader::DdsHeaderDx12), file, reinterpret_cast<void*>(const_cast<wchar_t *>(filename)),
			[](void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* buffer, File file)
		{
			const wchar_t* filename = reinterpret_cast<const wchar_t*>(requester);
			auto& virtualTextureManager = reinterpret_cast<SharedResources_t&>(sharedResources).virtualTextureManager;
			virtualTextureManager.loadTextureUncachedHelper(filename, file, executor, sharedResources, textureUseSubresource<SharedResources_t>,
				textureUploaded<SharedResources_t>, *reinterpret_cast<const DDSFileLoader::DdsHeaderDx12*>(buffer));
		});
	}

	void createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, RamToVramUploadRequest& vramRequest, const wchar_t* filename, File file);

	void unloadTextureHelper(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, StreamingManager& streamingManager);
public:
	VirtualTextureManager(D3D12GraphicsEngine& graphicsEngine) : pageProvider(-3.0f, graphicsEngine.adapter, graphicsEngine.graphicsDevice) {}
	~VirtualTextureManager() {}

	template<class Executor, class SharedResources_t>
	static void loadTexture(Executor* const executor, SharedResources_t& sharedResources, const wchar_t * filename, Request callback)
	{
		auto& virtualTextureManager = sharedResources.virtualTextureManager;
		auto& graphicsEngine = sharedResources.graphicsEngine;
		auto& streamingManager = executor->streamingManager;

		std::unique_lock<decltype(virtualTextureManager.mutex)> lock(virtualTextureManager.mutex);
		Texture& texture = virtualTextureManager.textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			virtualTextureManager.uploadRequests[filename].push_back(callback);
			lock.unlock();

			virtualTextureManager.loadTextureUncached<SharedResources_t>(filename, executor, sharedResources);
			return;
		}
		if (texture.loaded) //the resource is loaded
		{
			lock.unlock();
			callback(executor, sharedResources, texture);
			return;
		}
		//the resourse is loading
		virtualTextureManager.uploadRequests[filename].push_back(callback);
	}

	/*the texture must no longer be in use, including by the GPU*/
	template<class SharedResources_t>
	void unloadTexture(const wchar_t * filename, SharedResources_t& sharedResources)
	{
		unloadTextureHelper(filename, sharedResources.graphicsEngine, sharedResources.streamingManager);
	}
};