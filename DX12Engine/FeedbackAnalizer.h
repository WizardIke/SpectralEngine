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

class FeedbackAnalizerSubPass : public RenderSubPass<VirtualPageCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u,
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
	uint32_t lastFrameIndex;

	struct TextureLocationHasher : std::hash<uint64_t>
	{
		size_t operator()(TextureLocation location) const
		{
			return (*(std::hash<uint64_t>*)(this))(location.value);
		}
	};

	//contains all unique valid pages needed
	std::unordered_map<TextureLocation, PageRequestData, TextureLocationHasher> uniqueRequests;

	bool inView = true;

	template<class ThreadResources, class GlobalResources>
	static void readbackTextureReady(void* requester, void* tr, void* gr)
	{
		ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
		VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
		FeedbackAnalizerSubPass& analyser = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);

		gatherUniqueRequests(analyser, virtualTextureManager);

		//find needed pages and start them loading from disk if they aren't in the cache
		virtualTextureManager.pageProvider.processPageRequests(analyser.uniqueRequests, virtualTextureManager, threadResources, globalResources);
		analyser.uniqueRequests.clear();
	}

	static void gatherUniqueRequests(FeedbackAnalizerSubPass& subPass, VirtualTextureManager& virtualTextureManager);
	void createResources(D3D12GraphicsEngine& graphicsEngine, Transform& mainCameraTransform, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1, uint32_t width, uint32_t height, float fieldOfView);
public:
	float mipBias;
	float desiredMipBias;

	FeedbackAnalizerSubPass() {}
	FeedbackAnalizerSubPass(D3D12GraphicsEngine& graphicsEngine, Transform& mainCameraTransform, uint32_t width, uint32_t height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
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

		void update1AfterFirstThread(D3D12GraphicsEngine& graphicsEngine, FeedbackAnalizerSubPass& renderSubPass,
			ID3D12RootSignature* rootSignature, uint32_t barrierCount, D3D12_RESOURCE_BARRIER* barriers);

		template<class Executor, class SharedResources_t>
		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, FeedbackAnalizerSubPass& renderSubPass, Executor& executor, SharedResources_t& sharedResources)
		{
			addBarrier<state, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>(renderSubPass, lastCommandList());
			renderSubPass.lastFrameIndex = sharedResources.graphicsEngine.frameIndex;
			update2(commandLists, numThreads);

			executor.taskShedular.update1NextQueue().push({ &renderSubPass, [](void* context, Executor& executor, SharedResources_t& sharedResources)
			{
				FeedbackAnalizerSubPass& renderSubPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(context);
				renderSubPass.inView = false;
				StreamingManager& streamingManager = sharedResources.streamingManager;
				D3D12GraphicsEngine& graphicsEngine = sharedResources.graphicsEngine;
				StreamingManager::ThreadLocal& streamingManagerLocal = executor.streamingManager;

				streamingManager.commandQueue()->Wait(graphicsEngine.directFences[renderSubPass.lastFrameIndex], graphicsEngine.fenceValues[renderSubPass.lastFrameIndex]); //must be delayed after update2 so the command lists have been executed.

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
				streamingManagerLocal.addCopyCompletionEvent(context, readbackTextureReady<Executor, SharedResources_t>);
			} });
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter>
		static void addBarrier(FeedbackAnalizerSubPass& renderSubPass, ID3D12GraphicsCommandList* lastCommandList)
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