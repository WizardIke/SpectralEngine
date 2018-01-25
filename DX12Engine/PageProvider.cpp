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
	adapter->QueryVideoMemoryInfo(1u, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
	maxPages = ((signed long long)videoMemoryInfo.Budget - (signed long long)videoMemoryInfo.CurrentUsage) / (64 * 1024);
	memoryUsage = videoMemoryInfo.CurrentUsage;

	mipBias = desiredMipBias;
	this->desiredMipBias = desiredMipBias;

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
	uploadResource = D3D12Resource(graphicsDevice, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
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
	VirtualTextureInfo& resourceInfo = virtualTextureManager.texturesByID.data()[pageRequest.allocationInfo.textureLocation.textureId1()];

	PageProvider& pageProvider = *pageRequest.pageProvider;
	size_t pageRequestIndex = &pageRequest - pageProvider.pageLoadRequests;
	ScopedFile file(resourceInfo.fileName, ScopedFile::accessRight::genericRead, ScopedFile::shareMode::writeMode, ScopedFile::creationMode::openExisting, nullptr);
	unsigned int mipLevel = (unsigned int)pageRequest.allocationInfo.textureLocation.mipLevel();
	signed long long filePos = 0;
	auto width = resourceInfo.widthInTexels;
	auto height = resourceInfo.heightInTexels;
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
	filePos += resourceInfo.headerSize + rowBytes * ((pageRequest.allocationInfo.textureLocation.y() * resourceInfo.heightInTexels) / resourceInfo.heightInPages) +
		(pageRequest.allocationInfo.textureLocation.x() * resourceInfo.widthInTexels) / resourceInfo.widthInPages;
	file.setPosition(filePos, ScopedFile::Position::start);
	file.read(pageProvider.uploadResourcePointer + 64u * 1024u * pageRequestIndex, 64u * 1024u);
	pageRequest.state.store(PageProvider::PageLoadRequest::State::finished, std::memory_order::memory_order_release);
}