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

	for (auto& request : pageLoadRequests)
	{
		request.state.store(PageLoadRequest::State::unused, std::memory_order::memory_order_relaxed);
	}
}

//textures must be tiled
void PageProvider::addPageLoadRequestHelper(PageLoadRequest& pageRequest, VirtualTextureManager& virtualTextureManager, SharedResources& sharedResources)
{
	VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID[pageRequest.allocationInfo.textureLocation.textureId1()];
	pageRequest.filename = resourceInfo.filename;
	size_t mipLevel = (size_t)pageRequest.allocationInfo.textureLocation.mipLevel();
	auto width = resourceInfo.width;
	auto height = resourceInfo.height;
	uint64_t filePos = sizeof(DDSFileLoader::DdsHeaderDx12);
	for (size_t i = 0u; i != mipLevel; ++i)
	{
		size_t numBytes, rowBytes, numRows;
		DDSFileLoader::surfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
		filePos += numBytes;
		width = width >> 1u;
		if (width == 0u) width = 1u;
		height = height >> 1u;
		if (height == 0u) height = 1u;
	}
	size_t numBytes, rowBytes, numRows;
	DDSFileLoader::surfaceInfo(width, height, resourceInfo.format, numBytes, rowBytes, numRows);
	constexpr size_t pageSize = 64u * 1024u;
	auto heightInPages = resourceInfo.heightInPages >> mipLevel;
	if (heightInPages == 0u) heightInPages = 1u;
	auto widthInPages = resourceInfo.widthInPages >> mipLevel;
	if (widthInPages == 0u) widthInPages = 1u;
	auto pageX = pageRequest.allocationInfo.textureLocation.x();
	auto pageY = pageRequest.allocationInfo.textureLocation.y();
	
	RamToVramUploadRequest request;

	uint32_t pageHeightInTexels, pageWidthInTexels, pageWidthInBytes;
	DDSFileLoader::tileWidthAndHeightAndTileWidthInBytes(resourceInfo.format, pageWidthInTexels, pageHeightInTexels, pageWidthInBytes);

	filePos += pageY * rowBytes * pageHeightInTexels;
	if (pageY != (heightInPages - 1u))
	{
		filePos += pageSize * pageX;
		if (pageX < widthInPages)
		{
			sharedResources.streamingManager.addUploadRequest(request);
			request.meshInfo.indicesSize = pageWidthInBytes;
			request.meshInfo.verticesSize = pageHeightInTexels;
		}
		else
		{
			uint32_t lastColumnWidthInBytes = pageWidthInBytes - (widthInPages * pageWidthInBytes - (uint32_t)rowBytes);
			request.meshInfo.indicesSize = lastColumnWidthInBytes;
			request.meshInfo.verticesSize = pageHeightInTexels;
		}
	}
	else
	{
		uint32_t bottomPageHeight = pageHeightInTexels - (heightInPages * pageHeightInTexels - (uint32_t)numRows);
		uint32_t bottomPageSize = pageWidthInBytes * bottomPageHeight;
		filePos += bottomPageSize * pageX;
		if (pageX != (widthInPages - 1u))
		{
			request.meshInfo.indicesSize = pageWidthInBytes;
			request.meshInfo.verticesSize = bottomPageHeight;
		}
		else
		{
			uint32_t lastColumnWidthInBytes = pageWidthInBytes - (widthInPages * pageWidthInBytes - (uint32_t)rowBytes);
			request.meshInfo.indicesSize = lastColumnWidthInBytes;
			request.meshInfo.verticesSize = bottomPageHeight;
		}
	}

	pageRequest.offsetInFile = filePos;
	
	request.meshInfo.padding = pageWidthInBytes;
	request.arraySize = 1u;
	request.currentArrayIndex = 0u;
	request.currentMipLevel = 0u;
	request.dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	request.file = resourceInfo.file;
	request.meshInfo.sizeInBytes = pageSize;
	request.mostDetailedMip = 0u;
	request.requester = &pageRequest;
	
	request.useSubresource = [](BaseExecutor* executor, SharedResources& sharedResources, HalfFinishedUploadRequest& useSubresourceRequest)
	{
		auto& uploadRequest = *useSubresourceRequest.uploadRequest;
		const size_t length = uploadRequest.meshInfo.indicesSize * uploadRequest.meshInfo.verticesSize;
		PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(uploadRequest.requester);
		sharedResources.asynchronousFileManager.readFile(executor, sharedResources, pageRequest.filename, pageRequest.offsetInFile, pageRequest.offsetInFile + length, uploadRequest.file, &useSubresourceRequest,
			[](void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* data, File file)
		{
			HalfFinishedUploadRequest& useSubresourceRequest = *reinterpret_cast<HalfFinishedUploadRequest*>(requester);
			auto& uploadRequest = *useSubresourceRequest.uploadRequest;
			size_t widthInBytes = uploadRequest.meshInfo.indicesSize;
			size_t heightInTexels = uploadRequest.meshInfo.verticesSize;
			size_t pageWidthInBytes = (size_t)uploadRequest.meshInfo.padding;
			if (widthInBytes == pageWidthInBytes)
			{
				memcpy(useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos, data, widthInBytes * heightInTexels);
			}
			else
			{
				const uint8_t* source = data;
				uint8_t* destination = (uint8_t*)useSubresourceRequest.uploadBufferCpuAddressOfCurrentPos;
				for (std::size_t i = 0u; i != heightInTexels; ++i)
				{
					memcpy(destination, source, widthInBytes);
					source += widthInBytes;
					destination += pageWidthInBytes;
				}
			}

			PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(uploadRequest.requester);
			pageRequest.subresourceRequest = &useSubresourceRequest;
		});
	};

	request.resourceUploadedPointer = [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
	{
		PageLoadRequest& pageRequest = *reinterpret_cast<PageLoadRequest*>(requester);
		pageRequest.state.store(PageProvider::PageLoadRequest::State::finished, std::memory_order::memory_order_release);
	};
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
		commandList->CopyTiles(resource, &newPageCoordinates[i], &tileSize, pageLoadRequests[uploadBufferOffsets[i]].subresourceRequest->uploadRequest->uploadResource,
			pageLoadRequests[uploadBufferOffsets[i]].subresourceRequest->uploadResourceOffset,
			D3D12_TILE_COPY_FLAGS::D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);

		gpuCompletionEventManager.addRequest(&pageLoadRequests[uploadBufferOffsets[i]], [](void* requester, BaseExecutor* exe, SharedResources& sr)
		{
			PageLoadRequest* request = reinterpret_cast<PageLoadRequest*>(requester);
			request->state.store(PageLoadRequest::State::unused, std::memory_order::memory_order_release);
			request->subresourceRequest->copyFenceValue.store(0u, std::memory_order::memory_order_release); //copy has finished.
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