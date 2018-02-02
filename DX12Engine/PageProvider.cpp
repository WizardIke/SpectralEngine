#include "PageProvider.h"
#include "VirtualTextureManager.h"
#include <d3d12.h>
#include "BaseExecutor.h"
#include "SharedResources.h"
#include <d3d12.h>
#include "ScopedFile.h"

PageProvider::PageProvider(float desiredMipBias, IDXGIAdapter3* adapter, ID3D12Device* graphicsDevice)
{
	for (auto& pageRequest : pageLoadRequests)
	{
		pageRequest.state = PageLoadRequest::State::unused;
	}

	DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
	auto result = adapter->QueryVideoMemoryInfo(0u, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
	assert(result == S_OK);
	maxPages = ((signed long long)videoMemoryInfo.Budget - (signed long long)videoMemoryInfo.CurrentUsage) / (64 * 1024);
	memoryUsage = videoMemoryInfo.CurrentUsage;

	mipBias = desiredMipBias;
	this->desiredMipBias = desiredMipBias;

	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 0u;
	uploadHeapProperties.VisibleNodeMask = 0u;

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	resourceDesc.Width = 64 * 1024 * maxPagesLoading;
	resourceDesc.Height = 1u;

	uploadResource = D3D12Resource(graphicsDevice, uploadHeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	D3D12_RANGE readRange{ 0u, 0u };
	uploadResource->Map(0u, &readRange, reinterpret_cast<void**>(&uploadResourcePointer));

	for (auto& request : pageLoadRequests)
	{
		request.pageProvider = this;
		request.state.store(PageLoadRequest::State::unused, std::memory_order::memory_order_relaxed);
	}
}

void PageProvider::addPageLoadRequestHelper(PageLoadRequest& pageRequest, VirtualTextureManager& virtualTextureManager)
{
	VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID[pageRequest.allocationInfo.textureLocation.textureId1()];

	PageProvider& pageProvider = *pageRequest.pageProvider;
	size_t pageRequestIndex = &pageRequest - pageProvider.pageLoadRequests;
	ScopedFile file(resourceInfo.fileName, ScopedFile::accessRight::genericRead, ScopedFile::shareMode::readMode, ScopedFile::creationMode::openExisting, nullptr);
	unsigned int mipLevel = (unsigned int)pageRequest.allocationInfo.textureLocation.mipLevel();
	signed long long filePos = resourceInfo.headerSize;
	auto width = resourceInfo.pageWidthInTexels * resourceInfo.widthInPages;
	auto height = resourceInfo.pageHeightInTexels * resourceInfo.heightInPages;
	for (auto i = 0u; i != mipLevel; ++i)
	{
		size_t numBytes, rowBytes, numRows;
		DDSFileLoader::getSurfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
		filePos += numBytes;
		width = width >> 1;
		if (width == 0u) width = 1;
		height = height >> 1;
		if (height == 0u) height = 1;
	}
	size_t numBytes, rowBytes, numRows;
	DDSFileLoader::getSurfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
	size_t bitsPerPixel = DDSFileLoader::bitsPerPixel(resourceInfo.format);
	filePos += pageRequest.allocationInfo.textureLocation.y() * resourceInfo.pageHeightInTexels * rowBytes;
	filePos += (pageRequest.allocationInfo.textureLocation.x() * resourceInfo.pageWidthInTexels * bitsPerPixel) / 8u;
	file.setPosition(filePos, ScopedFile::Position::start);
	//might be better to use a file format that supports tiles
	//warning this presumes that width of texture is an exact number of tiles
	uint32_t pageWidthInBytes = uint32_t((resourceInfo.pageWidthInTexels * bitsPerPixel) / 8u);
	size_t filePosIncrease = rowBytes - pageWidthInBytes;
	uint8_t* uploadBufferPos = pageProvider.uploadResourcePointer + 64u * 1024u * pageRequestIndex;
	for (unsigned int i = 0u; i != resourceInfo.pageHeightInTexels; ++i)
	{
		file.read(uploadBufferPos, pageWidthInBytes);
		file.setPosition(filePosIncrease, ScopedFile::Position::current);
		uploadBufferPos += pageWidthInBytes;
	}
	pageRequest.state.store(PageProvider::PageLoadRequest::State::finished, std::memory_order::memory_order_release);
}

void PageProvider::addPageDataToResource(ID3D12Resource* resource, D3D12_TILED_RESOURCE_COORDINATE* newPageCoordinates, unsigned int pageCount,
	D3D12_TILE_REGION_SIZE& tileSize, unsigned long* uploadBufferOffsets, ID3D12GraphicsCommandList* commandList, GpuCompletionEventManager& gpuCompletionEventManager, uint32_t frameIndex)
{
	D3D12_RESOURCE_BARRIER barriers[1];
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[0].Transition.pResource = resource;
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1u, barriers);
	for (auto i = 0u; i != pageCount; ++i)
	{
		commandList->CopyTiles(resource, &newPageCoordinates[i], &tileSize, uploadResource, uploadBufferOffsets[i] * 64u * 1024u,
			D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);

		gpuCompletionEventManager.addRequest(&pageLoadRequests[uploadBufferOffsets[i]], [](void* requester, BaseExecutor* exe, SharedResources& sr)
		{
			PageLoadRequest* request = reinterpret_cast<PageLoadRequest*>(requester);
			request->state.store(PageLoadRequest::State::unused, std::memory_order::memory_order_release);
		}, frameIndex);
	}

	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	commandList->ResourceBarrier(1u, barriers);
}

bool PageProvider::NewPageIterator::NewPage::operator<(const NewPage& other)
{
	VirtualTextureInfo* resourceInfo = &virtualTextureManager.texturesByID[page->textureLocation.textureId1()];
	VirtualTextureInfo* otherResourceInfo = &virtualTextureManager.texturesByID[other.page->textureLocation.textureId1()];
	return resourceInfo < otherResourceInfo;
}