#include "TextureManager.h"
#include "DDSFileLoader.h"
#include "TextureIndexOutOfRangeException.h"
#include "SharedResources.h"
#include "BaseExecutor.h"
#include <d3d12.h>

TextureManager::TextureManager() {}
TextureManager::~TextureManager() {}

unsigned int TextureManager::loadTexture(const wchar_t * filename, void* requester, BaseExecutor* const executor, void(*resourceUploadedCallback)(void* const requester, BaseExecutor* const executor))
{
	TextureManager& textureManager = executor->sharedResources->textureManager;
	std::unique_lock<decltype(textureManager.mutex)> lock(textureManager.mutex);
	Texture& texture = textureManager.textures[filename];
	texture.numUsers += 1u;
	if (texture.numUsers == 1u)
	{
		textureManager.uploadRequests.insert({ filename, PendingLoadRequest{ requester, resourceUploadedCallback } });
		lock.unlock();

		return textureManager.loadTextureUncached(executor, filename);
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

void TextureManager::unloadTexture(const wchar_t * filename, BaseExecutor* const executor)
{
	unsigned int descriptorIndex = std::numeric_limits<unsigned int>::max();
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto& texture = textures[filename];
		texture.numUsers -= 1u;
		if (texture.numUsers == 0u)
		{
			descriptorIndex = texture.descriptorIndex;
			textures.erase(filename);
		}
	}
	if (descriptorIndex != std::numeric_limits<unsigned int>::max())
	{
		executor->sharedResources->graphicsEngine.descriptorAllocator.deallocate(descriptorIndex);
	}
}

unsigned int TextureManager::loadTextureUncached(BaseExecutor* const executor, const wchar_t * filename)
{
	RamToVramUploadRequest& uploadRequest = executor->streamingManager.getUploadRequest();
	uploadRequest.requester = reinterpret_cast<void*>(const_cast<wchar_t *>(filename));
	uploadRequest.resourceUploadedPointer = textureUploaded;
	uploadRequest.useSubresourcePointer = textureUseSubresource;

	auto& graphicsEngine = executor->sharedResources->graphicsEngine;

	D3D12_SHADER_RESOURCE_VIEW_DESC textureShaderResourceView;
	D3D12_RESOURCE_DESC textureResourceDesc;
	uploadRequest.file.open(filename, ScopedFile::accessRight::genericRead, ScopedFile::shareMode::readMode, ScopedFile::creationMode::openExisting, nullptr);
	DDSFileLoader::GetDDSTextureInfoFromFile(textureResourceDesc, uploadRequest.file, textureShaderResourceView);

	uploadRequest.width = static_cast<uint32_t>(textureResourceDesc.Width);
	uploadRequest.height = textureResourceDesc.Height;
	uploadRequest.format = textureResourceDesc.Format;

	graphicsEngine.graphicsDevice->GetCopyableFootprints(&textureResourceDesc, 0u, 1u, 0u, nullptr, nullptr, nullptr, &uploadRequest.uploadSizeInBytes);

	if (textureResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		uploadRequest.numSubresources = textureResourceDesc.MipLevels;
		uploadRequest.depth = textureResourceDesc.DepthOrArraySize;
	}
	else
	{
		uploadRequest.numSubresources = textureResourceDesc.DepthOrArraySize * textureResourceDesc.MipLevels;
		uploadRequest.depth = 1u;
	}

	auto discriptorIndex = graphicsEngine.descriptorAllocator.allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE discriptorHandle = graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	discriptorHandle.ptr += discriptorIndex * graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;

	D3D12_HEAP_PROPERTIES textureHeapProperties;
	textureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	textureHeapProperties.CreationNodeMask = 1u;
	textureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	textureHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	textureHeapProperties.VisibleNodeMask = 1u;

	D3D12Resource resource(graphicsEngine.graphicsDevice, textureHeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, textureResourceDesc,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr);

#ifdef _DEBUG
	wchar_t name[64] = L"texture ";
	wcscat_s(name, filename);
	resource->SetName(name);
#endif // _DEBUG

	ID3D12Resource* resourcePointer = resource;
	
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		Texture& texture = textures[filename];
		texture.resource = std::move(resource);
		texture.descriptorIndex = discriptorIndex;
	}

	graphicsEngine.graphicsDevice->CreateShaderResourceView(resourcePointer, &textureShaderResourceView, discriptorHandle);

	return discriptorIndex;
}

void TextureManager::textureUploaded(void* storedFilename, BaseExecutor* executor)
{
	const wchar_t* filename = reinterpret_cast<wchar_t*>(storedFilename);
	auto& textureManager = executor->sharedResources->textureManager;

	{
		std::lock_guard<decltype(textureManager.mutex)> lock(textureManager.mutex);
		auto& texture = textureManager.textures[filename];
		//resourcesBarrier.Transition.pResource = texture.resource;
		texture.loaded = true;

		while (true)
		{
			auto request = textureManager.uploadRequests.find(filename);
			if (request == textureManager.uploadRequests.end()) break;
			PendingLoadRequest requestCopy = request->second;
			textureManager.uploadRequests.erase(request);
			requestCopy.resourceUploaded(requestCopy.requester, executor);
		}
	}
}

void TextureManager::textureUseSubresource(RamToVramUploadRequest* const request, BaseExecutor* executor, void* const uploadBufferCpuAddressOfCurrentPos, ID3D12Resource* uploadResource,
	uint64_t uploadResourceOffset)
{
	auto& textureManager = executor->sharedResources->textureManager;
	ID3D12Resource* texture;
	const wchar_t* filename = reinterpret_cast<wchar_t*>(request->requester);
	{
		std::lock_guard<decltype(textureManager.mutex)> lock(textureManager.mutex);
		texture = textureManager.textures[filename].resource;
	}

	DDSFileLoader::LoadDDsSubresourceFromFile(executor->sharedResources->graphicsEngine.graphicsDevice, request->width, request->height, request->depth, request->format, request->file, uploadBufferCpuAddressOfCurrentPos,
		texture, executor->streamingManager.currentCommandList, request->currentSubresourceIndex, uploadResource, uploadResourceOffset);
}