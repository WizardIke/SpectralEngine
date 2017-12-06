#pragma once

#include "../Array/Array.h"
#include "D3D12Resource.h"
#include "RamToVramUploadRequest.h"
#include <unordered_map>
#include <mutex>
class BaseExecutor;
class SharedResources;

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
	
	unsigned int loadTextureUncached(BaseExecutor* const executor, const wchar_t * filename);

	static void textureUseSubresource(RamToVramUploadRequest* const request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
		uint64_t uploadResourceOffset);

	static void textureUploaded(void* storedFilename, BaseExecutor* executor);
public:
	TextureManager();
	~TextureManager();

	static unsigned int loadTexture(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor));

	/*the texture must no longer be in use, including by the GPU*/
	void unloadTexture(const wchar_t * filename, BaseExecutor* const executor);
};