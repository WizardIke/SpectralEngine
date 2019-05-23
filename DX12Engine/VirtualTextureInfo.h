#pragma once
#include <d3d12.h>
#include "File.h"
#include "GpuHeapLocation.h"
#include "PageCachePerTextureData.h"
#include "D3D12Resource.h"
#include <memory>
#include "LinkedTask.h"

struct VirtualTextureInfo
{
private:
	friend class PageProvider;
	class UnloadRequest : private LinkedTask
	{
		friend class PageProvider;
		PageProvider* pageProvider;
	public:
		VirtualTextureInfo* textureInfo;
		void(*callback)(UnloadRequest& unloadRequest, void* tr, void* gr);

		UnloadRequest() {}
		UnloadRequest(VirtualTextureInfo& textureInfo1, void(&callback1)(UnloadRequest& unloadRequest, void* tr, void* gr)) :
			textureInfo(&textureInfo1),
			callback(callback1) {}
	};
public:
	D3D12Resource resource;
	unsigned int widthInPages;
	unsigned int heightInPages;
	unsigned int numMipLevels;
	unsigned int lowestPinnedMip;
	DXGI_FORMAT format;
	unsigned int width;
	unsigned int height;
	unsigned int textureID;
	unsigned long pinnedPageCount;
	File file;
	const wchar_t* filename;
	std::unique_ptr<GpuHeapLocation[]> pinnedHeapLocations;
	PageCachePerTextureData pageCacheData;
	UnloadRequest* unloadRequest;
};