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

	VirtualFeedbackSubPass(void* taskShedular, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager, Transform& mainCameraTransform, float fieldOfView, Paremeters paremeters);
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
				barriers[barrierCount].Transition.pResource = camera.getImage();
				barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barriers[barrierCount].Transition.StateBefore = stateBefore;
				barriers[barrierCount].Transition.StateAfter = stateAfter;
				++barrierCount;
			}
			lastCommandList->ResourceBarrier(barrierCount, barriers);
		}
	};
};