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

class VirtualTextureManager
{
public:
	class Request
	{
		friend class VirtualTextureManager;
		unsigned int slot;
		void* requester;
		void(*job)(void*const requester, BaseExecutor* const executor, unsigned int textureDescriptorIndex, unsigned int textureID);
	public:
		Request(void* const requester, void(*job)(void*const requester, BaseExecutor* const executor, unsigned int textureDescriptorIndex, unsigned int textureID), unsigned int slot) : requester(requester),
			job(job), slot(slot) {}
		Request() {}

		void operator()(BaseExecutor* const executor, unsigned int textureIndex, unsigned int textureID)
		{
			job(requester, executor, textureIndex, textureID);
		}
	};
private:
	struct Texture
	{
		Texture() : resource(), numUsers(0u), loaded(false) {}
		ID3D12Resource* resource;
		unsigned int descriptorIndex;
		unsigned int textureID;
		unsigned int numUsers;
		bool loaded;
	};

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
	TextureInfoAllocator texturesByIDAndSlot[textureLocation::maxTextureSlots];
	PageProvider pageProvider;
private:

	template<class Executor>
	static void textureUploaded(void* storedFilename, BaseExecutor* exe)
	{
		const auto executor = reinterpret_cast<Executor*>(exe);
		auto& virtualTextureManager = executor->sharedResources->virtualTextureManager;

		virtualTextureManager.textureUploadedHelper(storedFilename, exe);
	}

	void textureUploadedHelper(void* storedFilename, BaseExecutor* executor);

	void loadTextureUncachedHelper(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, const wchar_t * filename,
		void(*textureUseSubresource)(RamToVramUploadRequest* const request, BaseExecutor* executor1, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
			uint64_t uploadResourceOffset), void(*textureUploaded)(void* storedFilename, BaseExecutor* exe));

	void textureUseSubresourceHelper(RamToVramUploadRequest& request, D3D12GraphicsEngine& graphicsEngine, StreamingManagerThreadLocal& streamingManager, void* const uploadBufferCpuAddressOfCurrentPos,
		ID3D12Resource* uploadResource, uint64_t uploadResourceOffset);

	template<class Executor>
	static void textureUseSubresource(RamToVramUploadRequest* const request, BaseExecutor* executor1, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset)
	{
		auto executor = reinterpret_cast<Executor*>(executor1);
		auto& virtualTextureManager = executor->sharedResources->virtualTextureManager;
		auto& graphicsEngine = executor->sharedResources->graphicsEngine;
		auto& streamingManager = executor->streamingManager;

		virtualTextureManager.textureUseSubresourceHelper(*request, graphicsEngine, streamingManager, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset);
	}

	template<class Executor>
	void loadTextureUncached(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, const wchar_t * filename)
	{
		loadTextureUncachedHelper(streamingManager, graphicsEngine, commandQueue, filename, textureUseSubresource<Executor>, textureUploaded<Executor>);
	}

	void createTextureWithResitencyInfo(D3D12GraphicsEngine& graphicsEngine, ID3D12CommandQueue* commandQueue, RamToVramUploadRequest& vramRequest, const wchar_t* filename);

	void unloadTextureHelper(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine, StreamingManager& streamingManager, unsigned int slot);
public:
	VirtualTextureManager(D3D12GraphicsEngine& graphicsEngine) : pageProvider(log2f(0.5f), graphicsEngine.adapter) {}
	~VirtualTextureManager() {}

	template<class Executor>
	static void loadTexture(Executor* const executor, const wchar_t * filename, Request callback)
	{
		auto& virtualTextureManager = executor->sharedResources->virtualTextureManager;
		auto& graphicsEngine = executor->sharedResources->graphicsEngine;
		auto& streamingManager = executor->streamingManager;

		std::unique_lock<decltype(virtualTextureManager.mutex)> lock(virtualTextureManager.mutex);
		Texture& texture = virtualTextureManager.textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			virtualTextureManager.uploadRequests[filename].push_back(callback);
			lock.unlock();

			virtualTextureManager.loadTextureUncached<Executor>(streamingManager, graphicsEngine, executor->sharedResources->StreamingManager.commandQueue(), filename, slot);
			return;
		}
		if (texture.loaded) //the resource is loaded
		{
			unsigned int textureIndex = texture.descriptorIndex;
			lock.unlock();
			callback(executor, textureIndex);
			return;
		}
		//the resourse is loading
		virtualTextureManager.uploadRequests[filename].push_back(callback);
	}

	/*the texture must no longer be in use, including by the GPU*/
	template<class SharedResources>
	void unloadTexture(const wchar_t * filename, SharedResources* sharedResources, unsigned int slot)
	{
		unloadTextureHelper(filename, sharedResources->graphicsEngine, sharedResources->streamingManager, slot);
	}
};