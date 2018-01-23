#include "FeedbackAnalizer.h"

void FeedbackAnalizerSubPass::readbackTextureReadyHelper(void* requester, VirtualTextureManager& virtualTextureManager, BaseExecutor* executor)
{
	FeedbackAnalizerSubPass& subPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);
	auto& uniqueRequests = subPass.uniqueRequests;
	uint8_t* feadBackBuffer = subPass.readbackTextureCpu;
	const auto packedRowPitch = subPass.packedRowPitch;
	const auto width = subPass.width;
	const auto height = subPass.height;
	const auto rowPitch = subPass.rowPitch;

	for (unsigned long y = 0u; y < height; ++y)
	{
		for (unsigned long x = 0u; x < packedRowPitch; x += 4)
		{
			unsigned long index = y * rowPitch + x;
			auto start = feadBackBuffer + index;
			textureLocation feedbackData;
			feedbackData.value = *reinterpret_cast<uint64_t*>(start);

			auto textureId = feedbackData.textureId();
			auto textureSlots = feedbackData.textureSlots();
			for (auto i = 0u; i < textureLocation::maxTextureSlots; ++i)
			{
				if ((textureSlots >> i) & 1)
				{
					VirtualTextureInfo& textureInfo = virtualTextureManager.texturesByIDAndSlot[i].data()[textureId];

					unsigned int nextMipLevel = (unsigned int)feedbackData.mipLevel();
					if (nextMipLevel < textureInfo.lowestPinnedMip)
					{
						feedbackData.setTextureSlots(i);
						PageRequestData& pageRequest = uniqueRequests[feedbackData];
						++pageRequest.count;
						++nextMipLevel;
						while (nextMipLevel != textureInfo.lowestPinnedMip)
						{
							feedbackData.setMipLevel(nextMipLevel);
							PageRequestData& pageRequest = uniqueRequests[feedbackData];
							++pageRequest.count;
							++nextMipLevel;
						}
					}
				}
			}
		}
	}
}

void FeedbackAnalizerSubPass::destruct(SharedResources* sharedResources)
{
	//sharedResources->graphicsEngine.descriptorAllocator.deallocate(feadbackTextureGpuDescriptorIndex);
	D3D12_RANGE writenRange{ 0u, 0u };
	readbackTexture->Unmap(0u, &writenRange);
}

void FeedbackAnalizerSubPass::ThreadLocal::addBarrier(FeedbackAnalizerSubPass& renderSubPass, ID3D12GraphicsCommandList* lastCommandList)
{
	auto cameras = renderSubPass.cameras();
	auto camerasEnd = cameras.end();
	uint32_t barrierCount = 0u;
	D3D12_RESOURCE_BARRIER barriers[1];
	for (auto cam = cameras.begin(); cam != camerasEnd; ++cam)
	{
		assert(barrierCount != 1);
		auto camera = *cam;
		barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[barrierCount].Transition.pResource = camera->getImage();
		barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barriers[barrierCount].Transition.StateBefore = state;
		barriers[barrierCount].Transition.StateAfter = stateAfter;
		++barrierCount;
	}
	lastCommandList->ResourceBarrier(barrierCount, barriers);
}

void FeedbackAnalizerSubPass::createResources(BaseExecutor* executor, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
	float mipBias)
{
	const auto sharedResources = executor->sharedResources;
	auto& graphicsEngine = sharedResources->graphicsEngine;
	ID3D12Device* graphicsDevice = graphicsEngine.graphicsDevice;

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptorHeapDesc.NumDescriptors = 1u;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 1u;
	depthStencilDescriptorHeap = D3D12DescriptorHeap(graphicsDevice, descriptorHeapDesc);

	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeap = D3D12DescriptorHeap(graphicsDevice, descriptorHeapDesc);


	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
	resourceDesc.Height = 1u;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	size_t numRows, packedSize, packedRowPitch;
	DDSFileLoader::getSurfaceInfo(width, height, resourceDesc.Format, packedSize, packedRowPitch, numRows);
	this->packedRowPitch = (unsigned long)packedRowPitch;
	rowPitch = (packedRowPitch + (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u);
	uint64_t totalSize = rowPitch * (numRows - 1u) + packedRowPitch;
	resourceDesc.Width = totalSize;

	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1u;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
	heapProperties.VisibleNodeMask = 1u;

	new(&readbackTexture) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr);
	D3D12_RANGE readRange{ 0u, totalSize };
	readbackTexture->Map(0u, &readRange, reinterpret_cast<void**>(&readbackTextureCpu));

	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Height = height;
	resourceDesc.Width = width;
	new(&feadbackTextureGpu) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0u;
	rtvDesc.Texture2D.PlaneSlice = 0u;

	auto rtvdescriptor = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	graphicsDevice->CreateRenderTargetView(feadbackTextureGpu, &rtvDesc, rtvdescriptor);

	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	new(&depthBuffer) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0u;

	auto depthDescriptor = depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	graphicsDevice->CreateDepthStencilView(depthBuffer, &dsvDesc, depthDescriptor);

	new(&camera) VirtualPageCamera(sharedResources, feadbackTextureGpu, rtvdescriptor, depthDescriptor, width, height, constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView,
		sharedResources->playerPosition.location, mipBias);
}