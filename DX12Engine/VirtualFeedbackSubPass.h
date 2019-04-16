#pragma once
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include <new>
#include "RenderSubPass.h"
#include "VirtualPageCamera.h"
#include "DDsFileLoader.h"
#include "StreamingManager.h"
#include "VirtualTextureManager.h"
#include "PageProvider.h"
#include "PageAllocationInfo.h"
#undef min
#undef max

class VirtualFeedbackSubPass : public RenderSubPass<VirtualPageCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u,
	D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, true, 1u>
{
	//friend class ThreadLocal;
	using Base = RenderSubPass<VirtualPageCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, true, 1u>;
	D3D12Resource feadbackTextureGpu;
	D3D12Resource depthBuffer;
	D3D12DescriptorHeap depthStencilDescriptorHeap;
	D3D12DescriptorHeap rtvDescriptorHeap;
	D3D12Resource readbackTexture;
	unsigned long textureWidth, textureHeight;
	bool inView = true;

	template<class ThreadResources, class GlobalResources>
	static void readbackTextureReady(void* requester, void* tr, void*)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		threadResources.taskShedular.pushBackgroundTask({requester, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
			VirtualFeedbackSubPass& analyser = *static_cast<VirtualFeedbackSubPass*>(requester);
			PageProvider& pageProvider = virtualTextureManager.pageProvider;
			VirtualTextureInfoByID& texturesByID = virtualTextureManager.texturesByID;

			const unsigned long totalSize = analyser.textureWidth * analyser.textureHeight * 8u;
			unsigned char* feadBackBuffer = analyser.mapReadbackTexture(totalSize);
			pageProvider.gatherPageRequests(feadBackBuffer, totalSize, texturesByID);
			analyser.unmapReadbackTexture();
			pageProvider.processPageRequests(texturesByID, threadResources, globalResources, analyser.mipBias, analyser.desiredMipBias);
		}});
	}

	unsigned char* mapReadbackTexture(unsigned long totalSize);
	void unmapReadbackTexture();
	void createResources(D3D12GraphicsEngine& graphicsEngine, Transform& mainCameraTransform, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1, uint32_t width, uint32_t height, float fieldOfView);
public:
	float mipBias;
	float desiredMipBias;

	VirtualFeedbackSubPass() {}
	VirtualFeedbackSubPass(D3D12GraphicsEngine& graphicsEngine, Transform& mainCameraTransform, uint32_t width, uint32_t height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
		unsigned char*& constantBufferCpuAddress1, float fieldOfView)
	{
		createResources(graphicsEngine, mainCameraTransform, constantBufferGpuAddress1, constantBufferCpuAddress1, width, height, fieldOfView);
	}

	void destruct();

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
		ThreadLocal(D3D12GraphicsEngine& graphicsEngine) : Base(graphicsEngine) {}

		void update1AfterFirstThread(D3D12GraphicsEngine& graphicsEngine, VirtualFeedbackSubPass& renderSubPass,
			ID3D12RootSignature* rootSignature, uint32_t barrierCount, D3D12_RESOURCE_BARRIER* barriers);

		template<class ThreadResources, class GlobalResources>
		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, VirtualFeedbackSubPass& renderSubPass, ThreadResources& executor, GlobalResources&)
		{
			addBarrier<state, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>(renderSubPass, lastCommandList());
			update2(commandLists, numThreads);

			executor.taskShedular.pushPrimaryTask(0u, { &renderSubPass, [](void* context, ThreadResources& executor, GlobalResources& sharedResources)
			{
				VirtualFeedbackSubPass& renderSubPass = *static_cast<VirtualFeedbackSubPass*>(context);
				renderSubPass.inView = false;
				StreamingManager& streamingManager = sharedResources.streamingManager;
				D3D12GraphicsEngine& graphicsEngine = sharedResources.graphicsEngine;
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
				streamingManagerLocal.addCopyCompletionEvent(context, readbackTextureReady<ThreadResources, GlobalResources>);
			} });
		}

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