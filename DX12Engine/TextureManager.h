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
		void(*job)(void*const requester, BaseExecutor* const executor, SharedResources& sharedResources, unsigned int textureDescriptorIndex);
	public:
		Request(const Request& other) = default;
		Request(void* const requester, void(*job)(void*const requester, BaseExecutor* const executor, SharedResources& sharedResources, unsigned int textureDescriptorIndex)) : requester(requester),
			job(job) {}
		Request() {}

		void operator()(BaseExecutor* const executor, SharedResources& sharedResources, unsigned int textureIndex)
		{
			job(requester, executor, sharedResources, textureIndex);
		}
	};

	std::mutex mutex;
	std::unordered_map<const wchar_t * const, std::vector<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
	
	void textureUseSubresourceHelper(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest);

	static void textureUseSubresource(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		sharedResources.backgroundQueue.push(Job(&useSubresourceRequest, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			auto& textureManager = sharedResources.textureManager;
			textureManager.textureUseSubresourceHelper(executor, sharedResources, *reinterpret_cast<HalfFinishedUploadRequest*>(requester));
		}));
	}

	static void textureUploaded(void* storedFilename, BaseExecutor* executor, SharedResources& sharedResources);

	void loadTextureUncached(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t * filename);

	static ID3D12Resource* createOrLookupTexture(const HalfFinishedUploadRequest& useSubresourceRequest, SharedResources& sharedResources, const wchar_t* filename);
public:
	TextureManager();
	~TextureManager();

	template<class Executor>
	static void loadTexture(Executor* executor, SharedResources& sharedResources, const wchar_t * filename, Request callback)
	{
		auto& textureManager = sharedResources.textureManager;

		std::unique_lock<decltype(textureManager.mutex)> lock(textureManager.mutex);
		Texture& texture = textureManager.textures[filename];
		texture.numUsers += 1u;
		if (texture.numUsers == 1u)
		{
			textureManager.uploadRequests[filename].push_back(callback);
			lock.unlock();

			textureManager.loadTextureUncached<Executor>(executor, sharedResources, filename);
			return;
		}
		//the resource is loaded or loading
		if (texture.loaded)
		{
			unsigned int textureIndex = texture.descriptorIndex;
			lock.unlock();
			callback(executor, sharedResources, textureIndex);
			return;
		}

		textureManager.uploadRequests[filename].push_back(callback);
		return;
	}

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine);
};