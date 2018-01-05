#pragma once

#include "../Array/Array.h"
#include "D3D12Resource.h"
#include "RamToVramUploadRequest.h"
#include <unordered_map>
#include <mutex>
#include "StreamingManager.h"
#include "DDSFileLoader.h"
class BaseExecutor;
class SharedResources;
class D3D12GraphicsEngine;

class TextureManager
{
private:
	/* Texture needs to store which mipmap levels are loaded, only some mipmap levels need loading*/
	struct Texture
	{
		Texture() : resource(), numUsers(0u), loaded(false) {}
		D3D12Resource resource;
		unsigned int descriptorIndex;
		unsigned int numUsers;
		bool loaded; //remove this bool
	};

	class Request
	{
		void* requester;
		void(*job)(void*const requester, BaseExecutor* const executor, unsigned int textureDescriptorIndex);
	public:
		Request(void* const requester, void(*job)(void*const requester, BaseExecutor* const executor, unsigned int textureDescriptorIndex)) : requester(requester),
			job(job) {}
		Request() {}

		void operator()(BaseExecutor* const executor, unsigned int textureIndex)
		{
			job(requester, executor, textureIndex);
		}
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, std::vector<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
	
	void loadTextureUncachedHelper(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t * filename,
		void(*textureUseSubresource)(RamToVramUploadRequest* const request, BaseExecutor* executor1, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
			uint64_t uploadResourceOffset));

	void textureUseSubresource(RamToVramUploadRequest& request, D3D12GraphicsEngine& graphicsEngine, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset, StreamingManagerThreadLocal& streamingManager);

	template<class Executor>
	static void textureUseSubresource(RamToVramUploadRequest* const request, BaseExecutor* executor1, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset)
	{
		auto executor = reinterpret_cast<Executor*>(executor1);
		auto sharedResources = executor->sharedResources;
		auto& textureManager = sharedResources->textureManager;
		textureManager.textureUseSubresource(*request, sharedResources->graphicsEngine, uploadBufferCpuAddressOfCurrentPos, uploadResource, uploadResourceOffset, executor->streamingManager);
	}

	static void textureUploaded(void* storedFilename, BaseExecutor* executor);

	template<class Executor>
	void TextureManager::loadTextureUncached(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t * filename)
	{
		loadTextureUncachedHelper(streamingManager, graphicsEngine, filename, textureUseSubresource<Executor>);
	}
public:
	TextureManager();
	~TextureManager();

	template<class Executor>
	static void loadTexture(Executor* const executor, const wchar_t * filename, Request callback)
	{
		auto& textureManager = executor->sharedResources->textureManager;
		auto& graphicsEngine = executor->sharedResources->graphicsEngine;
		auto& streamingManager = executor->streamingManager;

		std::unique_lock<decltype(textureManager.mutex)> lock(textureManager.mutex);
		Texture& texture = textureManager.textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			textureManager.uploadRequests[filename].push_back(callback);
			lock.unlock();

			textureManager.loadTextureUncached<Executor>(streamingManager, graphicsEngine, filename);
			return;
		}
		//the resource is loaded or loading
		if (texture.loaded)
		{
			unsigned int textureIndex = texture.descriptorIndex;
			lock.unlock();
			callback(executor, textureIndex);
			return;
		}

		textureManager.uploadRequests[filename].push_back(callback);
		return;
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine);
};