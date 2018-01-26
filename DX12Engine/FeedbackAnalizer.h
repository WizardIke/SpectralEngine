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

class FeedbackAnalizerSubPass : public RenderSubPass<VirtualPageCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u,
	D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>
{
	friend class ThreadLocal;
	using Base = RenderSubPass<VirtualPageCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>;
	D3D12Resource feadbackTextureGpu;
	D3D12Resource depthBuffer;
	D3D12DescriptorHeap depthStencilDescriptorHeap;
	D3D12DescriptorHeap rtvDescriptorHeap;
	D3D12Resource readbackTexture;
	//uint8_t* readbackTextureCpu;
	unsigned long width, packedRowPitch, height, rowPitch;
	VirtualPageCamera camera;
	unsigned long long memoryUsage;

	struct TextureLocationHasher : std::hash<uint64_t>
	{
		size_t operator()(textureLocation location) const
		{
			return (*(std::hash<uint64_t>*)(this))(location.value);
		}
	};

	//contains all unique valid pages needed
	std::unordered_map<textureLocation, PageRequestData, TextureLocationHasher> uniqueRequests;

	//unsigned int feadbackTextureGpuDescriptorIndex;
	bool inView = true;

	template<class Executor, class SharedResources_t>
	static void readbackTextureReady(void* requester, BaseExecutor* exe, SharedResources& sr)
	{
		Executor* executor = reinterpret_cast<Executor*>(exe);
		auto& sharedResources = reinterpret_cast<SharedResources_t&>(sr);
		VirtualTextureManager& virtualTextureManager = sharedResources.virtualTextureManager;
		readbackTextureReadyHelper(requester, virtualTextureManager, executor);

		FeedbackAnalizerSubPass& analyser = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);

		//find needed pages and start them loading from disk if they aren't in the cache
		virtualTextureManager.pageProvider.processPageRequests(analyser.uniqueRequests, virtualTextureManager, executor, sharedResources);
		analyser.uniqueRequests.clear();

		// we have finished with the readback resources so we can render again
		executor->updateJobQueue().push(Job(&analyser, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			FeedbackAnalizerSubPass& subPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);
			subPass.inView = true;
		}));
	}

	static void readbackTextureReadyHelper(void* requester, VirtualTextureManager& virtualTextureManager, BaseExecutor* executor);
	void createResources(SharedResources& sharedResources, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView);
public:
	FeedbackAnalizerSubPass() {}
	template<class RenderPass>
	FeedbackAnalizerSubPass(SharedResources& sharedResources, uint32_t width, uint32_t height, RenderPass& renderPass, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
		uint8_t*& constantBufferCpuAddress1, float fieldOfView) : width(width), height(height)
	{
		createResources(sharedResources, constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView);
		addCamera(sharedResources, renderPass, &camera);
	}

	void destruct(SharedResources* sharedResources);

	bool isInView(SharedResources& sharedResources)
	{
		return inView;
		//return false;
	}

	class ThreadLocal : public RenderSubPass<Camera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>::ThreadLocal
	{
		using Base = RenderSubPass<Camera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>::ThreadLocal;
	public:
		ThreadLocal(SharedResources& sharedResources) : Base(sharedResources) {}

		template<class Executor, class SharedResources_t>
		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, FeedbackAnalizerSubPass& renderSubPass, Executor* executor, SharedResources_t& sharedResources)
		{
			addBarrier(renderSubPass, lastCommandList());
			executor->updateJobQueue().push(Job(&renderSubPass, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources)
			{
				FeedbackAnalizerSubPass& renderSubPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);
				renderSubPass.inView = false; // this must be delayed after update2 because a thread could still be using this value in update1 which might not have finished fully

				executor->gpuCompletionEventManager.addRequest(&renderSubPass, [](void* requester, BaseExecutor* exe, SharedResources& sharedResources)
				{
					// copy feadbackTextureGpu to readbackTexture
					Executor* executor = reinterpret_cast<Executor*>(exe);
					StreamingManagerThreadLocal& streamingManager = executor->streamingManager;
					FeedbackAnalizerSubPass& renderSubPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);


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
					destination.PlacedFootprint.Footprint.Height = renderSubPass.height;
					destination.PlacedFootprint.Footprint.Width = renderSubPass.width;
					destination.PlacedFootprint.Footprint.RowPitch = static_cast<uint32_t>(renderSubPass.rowPitch);

					streamingManager.currentCommandList->CopyTextureRegion(&destination, 0u, 0u, 0u, &UploadBufferLocation, nullptr);

					streamingManager.addCopyCompletionEvent(requester, FeedbackAnalizerSubPass::readbackTextureReady<Executor, SharedResources_t>);
				}, sharedResources.graphicsEngine.frameIndex);
			}));
			
			update2(commandLists, numThreads);
		}

		static void addBarrier(FeedbackAnalizerSubPass& renderSubPass, ID3D12GraphicsCommandList* lastCommandList);
	};
};