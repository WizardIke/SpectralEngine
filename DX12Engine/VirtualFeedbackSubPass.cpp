#include "VirtualFeedbackSubPass.h"
#include "VirtualTextureInfo.h"

void* VirtualFeedbackSubPass::mapReadbackTexture(unsigned long totalSize)
{
	D3D12_RANGE readRange{0u, totalSize};
	void* feadBackBuffer;
	readbackTexture->Map(0u, &readRange, &feadBackBuffer);
	return feadBackBuffer;
}

void VirtualFeedbackSubPass::unmapReadbackTexture()
{
	D3D12_RANGE writtenRange{0u, 0u};
	readbackTexture->Unmap(0u, &writtenRange);
}

void VirtualFeedbackSubPass::destruct() {}

void VirtualFeedbackSubPass::createResources(GraphicsEngine& graphicsEngine, Transform& mainCameraTransform, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1,
	uint32_t width, uint32_t height, float fieldOfView)
{
	ID3D12Device* graphicsDevice = graphicsEngine.graphicsDevice;

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptorHeapDesc.NumDescriptors = 1u;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 1u;
	depthStencilDescriptorHeap = D3D12DescriptorHeap(graphicsDevice, descriptorHeapDesc);

	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeap = D3D12DescriptorHeap(graphicsDevice, descriptorHeapDesc);

	uint32_t newWidth = ((width / 4u) + 31u) & ~31;
	double widthMult = (double)newWidth / (double)width;
	uint32_t newHeight = (uint32_t)(height * widthMult);
	desiredMipBias = (float)std::log2(widthMult) - 1.0f;
	mipBias = desiredMipBias;
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	resourceDesc.Height = 1u;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	this->textureWidth = newWidth;
	this->textureHeight = newHeight;
	resourceDesc.Width = newWidth * newHeight * 8u;

	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1u;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
	heapProperties.VisibleNodeMask = 1u;

	new(&readbackTexture) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
	heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Height = newHeight;
	resourceDesc.Width = newWidth;
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 65280.0f;
	clearValue.Color[3] = 65535.0f;
	new(&feadbackTextureGpu) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, clearValue);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0u;
	rtvDesc.Texture2D.PlaneSlice = 0u;

	auto rtvdescriptor = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	graphicsDevice->CreateRenderTargetView(feadbackTextureGpu, &rtvDesc, rtvdescriptor);

	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
#ifndef NDEBUG
		; //graphics debugger can't handle resources with D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
#else
		| D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
#endif
	clearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0u;
	new(&depthBuffer) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, clearValue);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0u;

	auto depthDescriptor = depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	graphicsDevice->CreateDepthStencilView(depthBuffer, &dsvDesc, depthDescriptor);

	this->mCameras[0].init(feadbackTextureGpu, rtvdescriptor, depthDescriptor, newWidth, newHeight, constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView,
		mainCameraTransform);

#ifndef NDEBUG
	readbackTexture->SetName(L"Readback texture cpu");
	feadbackTextureGpu->SetName(L"Feedback texture gpu");
	depthBuffer->SetName(L"Feedback depth buffer");
#endif // !NDEBUG

}

void VirtualFeedbackSubPass::ThreadLocal::update1AfterFirstThread(GraphicsEngine& graphicsEngine, VirtualFeedbackSubPass& renderSubPass,
	ID3D12RootSignature* rootSignature, uint32_t barrierCount, D3D12_RESOURCE_BARRIER* barriers)
{
	auto frameIndex = graphicsEngine.frameIndex;
	resetCommandLists(frameIndex);
	bindRootArguments(rootSignature, graphicsEngine.mainDescriptorHeap);

	auto cameras = renderSubPass.cameras();
	auto camerasEnd = cameras.end();
	for (auto cam = cameras.begin(); cam != camerasEnd; ++cam)
	{
		auto& camera = *cam;
		barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[barrierCount].Transition.pResource = camera.getImage();
		barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barriers[barrierCount].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		barriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		++barrierCount;

		camera.render(graphicsEngine, renderSubPass.mipBias); //this function is called after waitForPreviousFrame so it is ok to use rendering functions
	}
	currentData->commandLists[0u]->ResourceBarrier(barrierCount, barriers);

	for (auto& camera : renderSubPass.cameras())
	{
		if (camera.isInView())
		{
			camera.bindFirstThread(frameIndex, &currentData->commandLists[0u].get(),
				&currentData->commandLists.data()[currentData->commandLists.size()].get());
		}
	}
}