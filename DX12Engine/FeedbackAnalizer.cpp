#include "FeedbackAnalizer.h"
#include <chrono>
/*
#include <Windows.h>
#include <iostream>
#include <sstream>

#define DBOUT( s )            \
{                             \
   std::wostringstream os_;    \
   os_ << s;                   \
   OutputDebugStringW( os_.str().c_str() );  \
}
*/

template<class HashMap>
static inline void requestMipLevels(unsigned int mipLevel, const VirtualTextureInfo* textureInfo, textureLocation feedbackData, HashMap& uniqueRequests)
{
	if (mipLevel < textureInfo->lowestPinnedMip)
	{
		auto x = feedbackData.x() >> mipLevel;
		auto y = feedbackData.y() >> mipLevel;
		feedbackData.setX(x);
		feedbackData.setY(y);

		PageRequestData& pageRequest = uniqueRequests[feedbackData];
		++pageRequest.count;
		++mipLevel;
		while (mipLevel != textureInfo->lowestPinnedMip)
		{
			x >>= 1u;
			y >>= 1u;
			feedbackData.setMipLevel(mipLevel);
			feedbackData.setX(x);
			feedbackData.setY(y);
			PageRequestData& pageRequest = uniqueRequests[feedbackData];
			++pageRequest.count;
			++mipLevel;
		}
	}
}

void FeedbackAnalizerSubPass::gatherUniqueRequests(FeedbackAnalizerSubPass& subPass, VirtualTextureManager& virtualTextureManager)
{
	auto& uniqueRequests = subPass.uniqueRequests;
	uint8_t* feadBackBuffer;

	const auto width = subPass.width;
	const auto height = subPass.height;
	const auto totalSize = width * height * 8u;
	D3D12_RANGE readRange{ 0u, totalSize };
	subPass.readbackTexture->Map(0u, &readRange, reinterpret_cast<void**>(&feadBackBuffer));

	//auto start = std::chrono::high_resolution_clock::now();

	const auto feadBackBufferEnd = feadBackBuffer + totalSize;
	for (; feadBackBuffer != feadBackBufferEnd; feadBackBuffer += 8u)
	{
		textureLocation feedbackData{*reinterpret_cast<uint64_t*>(feadBackBuffer)};

		auto textureId = feedbackData.textureId1();
		unsigned int nextMipLevel = (unsigned int)feedbackData.mipLevel();
		auto texture2d = feedbackData.textureId2();
		auto texture3d = feedbackData.textureId3();

		feedbackData.setTextureId2(0);
		feedbackData.setTextureId3(0);

		VirtualTextureInfo* textureInfo;
		if (textureId != 255u)
		{
			textureInfo = &virtualTextureManager.texturesByID[textureId];
			requestMipLevels(nextMipLevel, textureInfo, feedbackData, uniqueRequests);
		}
		if (texture2d != 255u)
		{
			feedbackData.setTextureId1(texture2d);
			textureInfo = &virtualTextureManager.texturesByID[texture2d];
			requestMipLevels(nextMipLevel, textureInfo, feedbackData, uniqueRequests);
		}
		if (texture3d != 255u)
		{
			feedbackData.setTextureId1(texture3d);
			textureInfo = &virtualTextureManager.texturesByID[texture3d];
			requestMipLevels(nextMipLevel, textureInfo, feedbackData, uniqueRequests);
		}
	}

	/*
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time = end - start;
	DBOUT("\n" << time.count());
	*/

	D3D12_RANGE writtenRange{ 0u, 0u };
	subPass.readbackTexture->Unmap(0u, &writtenRange);
}

void FeedbackAnalizerSubPass::destruct() {}

void FeedbackAnalizerSubPass::createResources(D3D12GraphicsEngine& graphicsEngine, Transform& mainCameraTransform, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1,
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

	uint32_t newWidth = (width + 31u) & ~31;
	double widthMult = (double)newWidth / (double)width;
	uint32_t newHeight = (uint32_t)(height * widthMult);
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
	this->width = newWidth;
	this->height = newHeight;
	resourceDesc.Width = newWidth * newHeight * 8u;

	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1u;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
	heapProperties.VisibleNodeMask = 1u;

	new(&readbackTexture) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr);

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
	new(&feadbackTextureGpu) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, &clearValue);

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
	new(&depthBuffer) D3D12Resource(graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue);

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

void FeedbackAnalizerSubPass::ThreadLocal::update1AfterFirstThread(D3D12GraphicsEngine& graphicsEngine, FeedbackAnalizerSubPass& renderSubPass,
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