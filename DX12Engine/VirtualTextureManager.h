#pragma once
class BaseExecutor;
#include "D3D12Resource.h"
#include <unordered_map>
#include <mutex>
#include <vector>
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
		unsigned int* freeList;
		unsigned int freeListEnd = 0u;
		unsigned int freeListCapacity = 0u;
		VirtualTextureInfo* mData;

		void resize();
	public:
		TextureInfoAllocator() = default;

		~TextureInfoAllocator()
		{
			delete[] freeList;
			delete[] mData;
		}

		unsigned int allocate()
		{
			if (freeListEnd == 0u)
			{
				resize();
			}
			--freeListEnd;
			return freeList[freeListEnd];
		}

		void deallocate(unsigned int index)
		{
			freeList[freeListEnd] = index;
			++freeListEnd;
		}

		VirtualTextureInfo* data() { return mData; }
		const VirtualTextureInfo* data() const { return mData; }
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, std::vector<Request>, std::hash<const wchar_t *>> uploadRequests;
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

	void loadTextureUncachedHelper(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, const wchar_t * filename,
		void(*textureUseSubresource)(RamToVramUploadRequest* const request, BaseExecutor* executor1, SharedResources& sharedResources, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
			uint64_t uploadResourceOffset), void(*textureUploaded)(void* storedFilename, BaseExecutor* exe, SharedResources& sharedResources));

	void textureUseSubresourceHelper(RamToVramUploadRequest& request, D3D12GraphicsEngine& graphicsEngine, StreamingManagerThreadLocal& streamingManager, void* const uploadBufferCpuAddressOfCurrentPos,
		ID3D12Resource* uploadResource, uint64_t uploadResourceOffset);

	template<class Executor, class SharedResources_t>
	static void textureUseSubresource(RamToVramUploadRequest* const request, BaseExecutor* executor1, SharedResources& sr,
		void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource, uint64_t uploadResourceOffset)
	{
		auto executor = reinterpret_cast<Executor*>(executor1);
		auto& sharedResources = reinterpret_cast<SharedResources_t&>(sr);
		auto& virtualTextureManager = sharedResources.virtualTextureManager;
		auto& graphicsEngine = sharedResources.graphicsEngine;
		auto& streamingManager = executor->streamingManager;

		virtualTextureManager.textureUseSubresourceHelper(*request, graphicsEngine, streamingManager, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset);
	}

	template<class Executor, class SharedResources_t>
	void loadTextureUncached(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, const wchar_t * filename)
	{
		loadTextureUncachedHelper(streamingManager, graphicsEngine, commandQueue, filename, textureUseSubresource<Executor, SharedResources_t>,
			textureUploaded<SharedResources_t>);
	}

	void createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, RamToVramUploadRequest& vramRequest, const wchar_t* filename);

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

			virtualTextureManager.loadTextureUncached<Executor, SharedResources_t>(streamingManager, graphicsEngine,
				sharedResources.streamingManager.commandQueue(), filename);
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