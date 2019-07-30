#pragma once
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include "RenderSubPass.h"
#include "VirtualPageCamera.h"
#include "DDsFileLoader.h"
#include "StreamingManager.h"
#include "VirtualTextureManager.h"
#include "PageProvider.h"
#include "PageAllocationInfo.h"
#include "TaskShedular.h"
#include "Tuple.h"
#undef min
#undef max

class VirtualFeedbackSubPass : public RenderSubPass<VirtualPageCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, Tuple<>, Tuple<>, 1u,
	D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, true, 1u>
{
	//friend class ThreadLocal;
	using Base = RenderSubPass<VirtualPageCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, Tuple<>, Tuple<>, 1u,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, true, 1u>;

	D3D12Resource feadbackTextureGpu;
	D3D12Resource depthBuffer;
	D3D12DescriptorHeap depthStencilDescriptorHeap;
	D3D12DescriptorHeap rtvDescriptorHeap;
	D3D12Resource readbackTexture;
	unsigned int textureWidth, textureHeight;
	bool inView = true;

	StreamingManager& streamingManager;
	GraphicsEngine& graphicsEngine;
	void* taskShedular;

	template<class ThreadResources>
	static void readbackTextureReady(void* requester, void* tr)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.pushBackgroundTask({requester, [](void* requester, ThreadResources& threadResources)
		{
			VirtualFeedbackSubPass& analyser = *static_cast<VirtualFeedbackSubPass*>(requester);
			PageProvider& pageProvider = analyser.pageProvider;

			const unsigned long totalSize = analyser.textureWidth * analyser.textureHeight * 8u;
			void* feadBackBuffer = analyser.mapReadbackTexture(totalSize);
			pageProvider.gatherPageRequests(feadBackBuffer, totalSize);
			analyser.unmapReadbackTexture();
			pageProvider.processPageRequests(threadResources, *static_cast<TaskShedular<ThreadResources>*>(analyser.taskShedular), analyser.mipBias, analyser.desiredMipBias);
		}});
	}

	void* mapReadbackTexture(unsigned long totalSize);
	void unmapReadbackTexture();

	struct Paremeters
	{
		D3D12Resource feadbackTextureGpu;
		D3D12DescriptorHeap rtvDescriptorHeap;
		D3D12Resource depthBuffer;
		D3D12DescriptorHeap depthStencilDescriptorHeap;
		uint32_t width;
		uint32_t height;
		float desiredMipBias;
	};

	VirtualFeedbackSubPass(void* taskShedular, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager, Transform& mainCameraTransform,
		float fieldOfView, Paremeters paremeters);

	class ResizeRequest : public LinkedTask
	{
		friend class VirtualFeedbackSubPass;
		VirtualFeedbackSubPass& subPass;
		unsigned int width;
		unsigned int height;
	public:
		ResizeRequest(void(*execute)(LinkedTask& task, void* tr), VirtualFeedbackSubPass& subPass1, unsigned int width1, unsigned int height1) :
			LinkedTask(execute),
			subPass(subPass1),
			width(width1),
			height(height1)
		{}
	};
public:
	PageProvider pageProvider;
	float mipBias;
	float desiredMipBias;

	VirtualFeedbackSubPass(void* taskShedular, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager, uint32_t screenWidth, uint32_t screenHeight,
		Transform& mainCameraTransform, float fieldOfView);
#if defined(_MSC_VER)
	/*
	Initialization of array members doesn't seam to have copy elision in some cases when it should in c++17.
	*/
	VirtualFeedbackSubPass(VirtualFeedbackSubPass&&);
#endif

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1);

	bool isInView()
	{
		return inView;
	}

	void setInView()
	{
		inView = true;
	}

	void resize(unsigned int width, unsigned int height)
	{
		ResizeRequest* request = new ResizeRequest([](LinkedTask& task, void*)
			{
				ResizeRequest& request = static_cast<ResizeRequest&>(task);
				auto screenWidth = request.width;
				auto screenHeight = request.height;
				auto& subPass = request.subPass;
				delete &request;


				subPass.textureWidth = ((screenWidth / 4u) + 31u) & ~31u;
				double widthMult = (double)subPass.textureWidth / (double)screenWidth;
				subPass.textureHeight = (uint32_t)(screenHeight * widthMult);

				subPass.desiredMipBias = (float)std::log2(widthMult) - 1.0f;

				subPass.feadbackTextureGpu = D3D12Resource(subPass.graphicsEngine.graphicsDevice, []()
					{
						D3D12_HEAP_PROPERTIES heapProperties;
						heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						heapProperties.CreationNodeMask = 1u;
						heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
						heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
						heapProperties.VisibleNodeMask = 1u;
						return heapProperties;
					}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [width = subPass.textureWidth, height = subPass.textureHeight]()
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

				subPass.depthBuffer = D3D12Resource(subPass.graphicsEngine.graphicsDevice, []()
					{
						D3D12_HEAP_PROPERTIES heapProperties;
						heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						heapProperties.CreationNodeMask = 1u;
						heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
						heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
						heapProperties.VisibleNodeMask = 1u;
						return heapProperties;
					}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [width = subPass.textureWidth, height = subPass.textureHeight]()
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

				subPass.readbackTexture = D3D12Resource(subPass.graphicsEngine.graphicsDevice, []()
					{
						D3D12_HEAP_PROPERTIES heapProperties;
						heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						heapProperties.CreationNodeMask = 1u;
						heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
						heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
						heapProperties.VisibleNodeMask = 1u;
						return heapProperties;
					}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, [width = subPass.textureWidth, height = subPass.textureHeight]()
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
					}(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
				rtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0u;
				rtvDesc.Texture2D.PlaneSlice = 0u;
				auto rtvdescriptor = subPass.rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				subPass.graphicsEngine.graphicsDevice->CreateRenderTargetView(subPass.feadbackTextureGpu, &rtvDesc, rtvdescriptor);

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
				dsvDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
				dsvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0u;
				auto depthDescriptor = subPass.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				subPass.graphicsEngine.graphicsDevice->CreateDepthStencilView(subPass.depthBuffer, &dsvDesc, depthDescriptor);

				auto& camera = *(subPass.cameras().begin());
				camera.resize(subPass.feadbackTextureGpu, subPass.textureWidth, subPass.textureHeight);
			}, *this, width, height);
		pageProvider.executeBeforeProcessPageRequests(*request);
	}

	class ThreadLocal : public Base::ThreadLocal
	{
		using Base = Base::ThreadLocal;
	public:
		ThreadLocal(GraphicsEngine& graphicsEngine) : Base(graphicsEngine) {}

		void update1AfterFirstThread(GraphicsEngine& graphicsEngine, VirtualFeedbackSubPass& renderSubPass,
			ID3D12RootSignature* rootSignature, uint32_t barrierCount, D3D12_RESOURCE_BARRIER* barriers);

		template<class ThreadResources>
		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, VirtualFeedbackSubPass& renderSubPass, ThreadResources& threadResources)
		{
			addBarrier<state, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>(renderSubPass, lastCommandList());
			update2(commandLists, numThreads);

			threadResources.taskShedular.pushPrimaryTask(0u, { &renderSubPass, [](void* context, ThreadResources& executor)
			{
				VirtualFeedbackSubPass& renderSubPass = *static_cast<VirtualFeedbackSubPass*>(context);
				renderSubPass.inView = false;
				StreamingManager& streamingManager = renderSubPass.streamingManager;
				GraphicsEngine& graphicsEngine = renderSubPass.graphicsEngine;
				StreamingManager::ThreadLocal& streamingManagerLocal = executor.streamingManager;

				graphicsEngine.waitForPreviousFrame(streamingManager.commandQueue()); //Should be delayed after update2 so the command lists have been executed.

				D3D12_TEXTURE_COPY_LOCATION UploadBufferLocation;
				UploadBufferLocation.pResource = renderSubPass.feadbackTextureGpu;
				UploadBufferLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				UploadBufferLocation.SubresourceIndex = 0u;

				D3D12_TEXTURE_COPY_LOCATION destination;
				destination.pResource = renderSubPass.readbackTexture;
				destination.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				destination.PlacedFootprint.Offset = 0u;
				destination.PlacedFootprint.Footprint.Depth = 1u;
				destination.PlacedFootprint.Footprint.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
				destination.PlacedFootprint.Footprint.Height = renderSubPass.textureHeight;
				destination.PlacedFootprint.Footprint.Width = renderSubPass.textureWidth;
				destination.PlacedFootprint.Footprint.RowPitch = static_cast<uint32_t>(renderSubPass.textureWidth * 8u);

				streamingManagerLocal.copyCommandList().CopyTextureRegion(&destination, 0u, 0u, 0u, &UploadBufferLocation, nullptr);
				streamingManagerLocal.addCopyCompletionEvent(context, readbackTextureReady<ThreadResources>);
			} });
		}
	private:
		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter>
		static void addBarrier(VirtualFeedbackSubPass& renderSubPass, ID3D12GraphicsCommandList* lastCommandList)
		{
			auto cameras = renderSubPass.cameras();
			auto camerasEnd = cameras.end();
			uint32_t barrierCount = 0u;
			D3D12_RESOURCE_BARRIER barriers[1];
			for (auto cam = cameras.begin(); cam != camerasEnd; ++cam)
			{
				assert(barrierCount != 1u);
				auto& camera = *cam;
				barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[barrierCount].Transition.pResource = &camera.getImage();
				barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barriers[barrierCount].Transition.StateBefore = stateBefore;
				barriers[barrierCount].Transition.StateAfter = stateAfter;
				++barrierCount;
			}
			lastCommandList->ResourceBarrier(barrierCount, barriers);
		}
	};
};