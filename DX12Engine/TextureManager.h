#pragma once

#include "ResizingArray.h"
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
	std::unordered_map<const wchar_t * const, ResizingArray<Request>, std::hash<const wchar_t *>> uploadRequests;
	std::unordered_map<const wchar_t * const, Texture, std::hash<const wchar_t *>> textures;
	
	void textureUseSubresourceHelper(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest);

	static void textureUseSubresource(BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest);

	static void textureUploaded(void* storedFilename, BaseExecutor* executor, SharedResources& sharedResources);

	void loadTextureUncached(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t * filename);

	static ID3D12Resource* createOrLookupTexture(const HalfFinishedUploadRequest& useSubresourceRequest, SharedResources& sharedResources, const wchar_t* filename);
public:
	TextureManager();
	~TextureManager();

	static void loadTexture(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t * filename, Request callback);

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t * filename, D3D12GraphicsEngine& graphicsEngine);
};