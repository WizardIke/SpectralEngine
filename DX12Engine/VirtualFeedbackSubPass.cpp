#include "VirtualFeedbackSubPass.h"
#include "VirtualTextureInfo.h"
#include "Tuple.h"

VirtualFeedbackSubPass::VirtualFeedbackSubPass(void* taskShedular, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager, uint32_t screenWidth, uint32_t screenHeight,
	Transform& mainCameraTransform, float fieldOfView) :
	VirtualFeedbackSubPass(taskShedular, streamingManager, graphicsEngine, asynchronousFileManager, mainCameraTransform, fieldOfView, [screenWidth, screenHeight, &graphicsEngine]()
		{
			Paremeters paremeters;

			paremeters.width = ((screenWidth / 4u) + 31u) & ~31u;
			double widthMult = (double)paremeters.width / (double)screenWidth;

			paremeters.height = (uint32_t)(screenHeight * widthMult);

			paremeters.desiredMipBias = (float)std::log2(widthMult) - 1.0f;

			paremeters.feadbackTextureGpu = D3D12Resource(graphicsEngine.graphicsDevice, []()
				{
					D3D12_HEAP_PROPERTIES heapProperties;
					heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					heapProperties.CreationNodeMask = 1u;
					heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
					heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
					heapProperties.VisibleNodeMask = 1u;
					return heapProperties;
				}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [width = paremeters.width, height = paremeters.height]()
				{
					D3D12_RESOURCE_DESC resourceDesc;
					resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
					resourceDesc.DepthOrArraySize = 1u;
					resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
					resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
					resourceDesc.Height = height;
					resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
					resourceDesc.MipLevels = 1u;
					resourceDesc.SampleDesc.Count = 1u;
					resourceDesc.SampleDesc.Quality = 0u;
					resourceDesc.Width = width;
					return resourceDesc;
				}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, []()
				{
					D3D12_CLEAR_VALUE clearValue;
					clearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
					clearValue.Color[0] = 0.0f;
					clearValue.Color[1] = 0.0f;
					clearValue.Color[2] = 65280.0f;
					clearValue.Color[3] = 65535.0f;
					return clearValue;
				}());

			paremeters.rtvDescriptorHeap = D3D12DescriptorHeap(graphicsEngine.graphicsDevice, []()
				{
					D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
					descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
					descriptorHeapDesc.NumDescriptors = 1u;
					descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					descriptorHeapDesc.NodeMask = 1u;
					return descriptorHeapDesc;
				}());

			paremeters.depthBuffer = D3D12Resource(graphicsEngine.graphicsDevice, []()
				{
					D3D12_HEAP_PROPERTIES heapProperties;
					heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					heapProperties.CreationNodeMask = 1u;
					heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
					heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
					heapProperties.VisibleNodeMask = 1u;
					return heapProperties;
				}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [width = paremeters.width, height = paremeters.height]()
				{
					D3D12_RESOURCE_DESC resourceDesc;
					resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
					resourceDesc.DepthOrArraySize = 1u;
					resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
					resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
					resourceDesc.Height = height;
					resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
					resourceDesc.MipLevels = 1u;
					resourceDesc.SampleDesc.Count = 1u;
					resourceDesc.SampleDesc.Quality = 0u;
					resourceDesc.Width = width;
					return resourceDesc;
				}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, []()
				{
					D3D12_CLEAR_VALUE clearValue;
					clearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
					clearValue.DepthStencil.Depth = 1.0f;
					clearValue.DepthStencil.Stencil = 0u;
					return clearValue;
				}());

			paremeters.depthStencilDescriptorHeap = D3D12DescriptorHeap(graphicsEngine.graphicsDevice, []()
				{
					D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
					descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
					descriptorHeapDesc.NumDescriptors = 1u;
					descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					descriptorHeapDesc.NodeMask = 1u;
					return descriptorHeapDesc;
				}());

			return paremeters;
		}())
{}

VirtualFeedbackSubPass::VirtualFeedbackSubPass(void* taskShedular, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager, Transform& mainCameraTransform, float fieldOfView,
	Paremeters paremeters) :
	Base{ Tuple<ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE, unsigned int, unsigned int, Transform&, float>{
		paremeters.feadbackTextureGpu, [&rtvDescriptorHeap = paremeters.rtvDescriptorHeap, &graphicsEngine, &feadbackTextureGpu = paremeters.feadbackTextureGpu]()
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0u;
			rtvDesc.Texture2D.PlaneSlice = 0u;

			auto rtvdescriptor = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			graphicsEngine.graphicsDevice->CreateRenderTargetView(feadbackTextureGpu, &rtvDesc, rtvdescriptor);
			return rtvdescriptor;
		}(), [&graphicsEngine, &depthStencilDescriptorHeap = paremeters.depthStencilDescriptorHeap, &depthBuffer = paremeters.depthBuffer]()
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			dsvDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
			dsvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0u;

			auto depthDescriptor = depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			graphicsEngine.graphicsDevice->CreateDepthStencilView(depthBuffer, &dsvDesc, depthDescriptor);
			return depthDescriptor;
		}(), paremeters.width, paremeters.height, mainCameraTransform, fieldOfView} },
	feadbackTextureGpu(std::move(paremeters.feadbackTextureGpu)),
	rtvDescriptorHeap(std::move(paremeters.rtvDescriptorHeap)),
	depthStencilDescriptorHeap(std::move(paremeters.depthStencilDescriptorHeap)),
	depthBuffer(std::move(paremeters.depthBuffer)),
	taskShedular(taskShedular),
	pageProvider(*this, streamingManager, graphicsEngine, asynchronousFileManager),
	streamingManager(streamingManager),
	graphicsEngine(graphicsEngine),
	textureWidth(paremeters.width),
	textureHeight(paremeters.height),
	mipBias(paremeters.desiredMipBias),
	desiredMipBias(paremeters.desiredMipBias),
	readbackTexture(graphicsEngine.graphicsDevice, []()
		{
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.CreationNodeMask = 1u;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
			heapProperties.VisibleNodeMask = 1u;
			return heapProperties;
		}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [width = paremeters.width, height = paremeters.height]()
		{
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
			resourceDesc.Width = width * height * 8u; //8 bytes in a DXGI_FORMAT_R16G16B16A16_UINT pixel
			return resourceDesc;
		}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST)
{
#ifndef NDEBUG
	readbackTexture->SetName(L"Readback texture cpu");
	feadbackTextureGpu->SetName(L"Feedback texture gpu");
	depthBuffer->SetName(L"Feedback depth buffer");
#endif // !NDEBUG
}

void VirtualFeedbackSubPass::setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1)
{
	mCameras[0].setConstantBuffers(constantBufferGpuAddress1, constantBufferCpuAddress1);
}

void* VirtualFeedbackSubPass::mapReadbackTexture(unsigned long totalSize)
{
	D3D12_RANGE readRange{ 0u, totalSize };
	void* feadBackBuffer;
	readbackTexture->Map(0u, &readRange, &feadBackBuffer);
	return feadBackBuffer;
}

void VirtualFeedbackSubPass::unmapReadbackTexture()
{
	D3D12_RANGE writtenRange{ 0u, 0u };
	readbackTexture->Unmap(0u, &writtenRange);
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

		camera.beforeRender(graphicsEngine, renderSubPass.mipBias); //this function is called after waitForPreviousFrame so it is ok to use rendering functions
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