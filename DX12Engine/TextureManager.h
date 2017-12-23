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
	struct PendingLoadRequest
	{
		void* requester;
		void(*resourceUploaded)(void* const requester, BaseExecutor* const executor);
	};

	/* Texture needs to store which mipmap levels are loaded, only some mipmap levels need loading*/
	struct Texture
	{
		Texture() : resource(), numUsers(0u), loaded(false) {}
		D3D12Resource resource;
		unsigned int descriptorIndex;
		unsigned int numUsers;
		bool loaded; //remove this bool
	};

	std::mutex mutex;
	std::unordered_multimap<const wchar_t * const, PendingLoadRequest, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
	
	unsigned int loadTextureUncachedHelper(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t * filename,
		void(*textureUseSubresource)(RamToVramUploadRequest* const request, BaseExecutor* executor1, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
			uint64_t uploadResourceOffset));

	template<class Executor>
	static void textureUseSubresource(RamToVramUploadRequest* const request, BaseExecutor* executor1, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset)
	{
		auto executor = reinterpret_cast<Executor*>(executor1);
		auto& textureManager = executor->sharedResources->textureManager;
		ID3D12Resource* texture;
		const wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
		{
			std::lock_guard<decltype(textureManager.mutex)> lock(textureManager.mutex);
			texture = textureManager.textures[filename].resource;
		}

		DDSFileLoader::LoadDDsSubresourceFromFile(executor->sharedResources->graphicsEngine.graphicsDevice, request->width, request->height, request->depth, request->format,
			request->file, uploadBufferCpuAddressOfCurrentPos, texture, executor->streamingManager.currentCommandList, request->currentSubresourceIndex, uploadResource,
			uploadResourceOffset);
	}

	static void textureUploaded(void* storedFilename, BaseExecutor* executor);

	template<class Executor>
	unsigned int TextureManager::loadTextureUncached(StreamingManagerThreadLocal& streamingManager, D3D12GraphicsEngine& graphicsEngine, const wchar_t * filename)
	{
		return loadTextureUncachedHelper(streamingManager, graphicsEngine, filename, textureUseSubresource<Executor>);
	}
public:
	TextureManager();
	~TextureManager();

	template<class Executor>
	static unsigned int loadTexture(const wchar_t * filename, void* requester, Executor* const executor,
		void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor))
	{
		auto& textureManager = executor->sharedResources->textureManager;
		auto& graphicsEngine = executor->sharedResources->graphicsEngine;
		auto& streamingManager = executor->streamingManager;

		std::unique_lock<decltype(textureManager.mutex)> lock(textureManager.mutex);
		Texture& texture = textureManager.textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			textureManager.uploadRequests.insert({ filename, PendingLoadRequest{ requester, resourceUploadedCallback } });
			lock.unlock();

			return textureManager.loadTextureUncached<Executor>(streamingManager, graphicsEngine, filename);
		}
		//the resource is loaded or loading
		if (texture.loaded)
		{
			unsigned int textureIndex = texture.descriptorIndex;
			lock.unlock();
			resourceUploadedCallback(requester, executor);
			return textureIndex;
		}
		else
		{
			textureManager.uploadRequests.insert({ filename, PendingLoadRequest{ requester, resourceUploadedCallback } });
			return texture.descriptorIndex;
		}
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine);
};