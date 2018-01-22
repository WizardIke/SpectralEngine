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
	uint8_t* readbackTextureCpu;
	unsigned long width, packedRowPitch, height, rowPitch;
	VirtualPageCamera camera;
	unsigned long long memoryUsage;

	//contains all unique valid pages needed
	std::unordered_map<textureLocation, PageRequestData> uniqueRequests;

	//unsigned int feadbackTextureGpuDescriptorIndex;
	bool inView = true;

	template<class Executor>
	static void readbackTextureReady(void* requester, BaseExecutor* exe)
	{
		Executor* executor = reinterpret_cast<Executor*>(exe);
		VirtualTextureManager& virtualTextureManager = executor->sharedResources->virtualTextureManager;
		readbackTextureReadyHelper(requester, virtualTextureManager);

		//find needed pages and start them loading from disk if they aren't in the cache
		virtualTextureManager.pageProvider.processPageRequests(uniqueRequests, executor->sharedResources->graphicsEngine.adapter, virtualTextureManager,
				executor);
		uniqueRequests.clear();

		// we have finished with the readback resources so we can render again
		executor->updateJobQueue().pop(Job(&subPass, [](void* requester, BaseExecutor* executor)
		{
			FeedbackAnalizerSubPass& subPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);
			subPass.inView = true;
		}));
	}

	static void readbackTextureReadyHelper(void* requester, VirtualTextureManager& virtualTextureManager, BaseExecutor* executor);
	void createResources(BaseExecutor* executor, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
		float mipBias);
public:
	template<class RenderPass>
	FeedbackAnalizerSubPass(BaseExecutor* executor, uint32_t width, uint32_t height, RenderPass& renderPass, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
		uint8_t*& constantBufferCpuAddress1, float fieldOfView, float mipBias) : width(width), height(height)
	{
		createResources(executor, constantBufferGpuAddress1, constantBufferCpuAddress1, fieldOfView, mipBias);
		addCamera(executor, renderPass, &camera);
	}

	void destruct(SharedResources* sharedResources);

	bool isInView(BaseExecutor* executor)
	{
		return inView;
	}

	class ThreadLocal : public RenderSubPass<Camera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>::ThreadLocal
	{
		using Base = RenderSubPass<Camera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON>::ThreadLocal;
	public:
		template<class Executor>
		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, FeedbackAnalizerSubPass& renderSubPass, Executor* executor)
		{
			addBarrier(renderSubPass, lastCommandList());
			executor->updateJobQueue().push(Job(&renderSubPass, [](void* requester, BaseExecutor* executor)
			{
				FeedbackAnalizerSubPass& renderSubPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);
				renderSubPass.inView = false; // this must be delayed after update2 because a thread could still be using this value in update1 which might not have finished fully

				executor->gpuCompletionEventManager.addRequest(&renderSubPass, [](void* requester, BaseExecutor* executor)
				{
					// copy feadbackTextureGpu to readbackTexture
					StreamingManagerThreadLocal& streamingManager = executor->streamingManager;
					FeedbackAnalizerSubPass& renderSubPass = *reinterpret_cast<FeedbackAnalizerSubPass*>(requester);
					streamingManager.currentCommandList->CopyResource(renderSubPass.readbackTexture, renderSubPass.feadbackTextureGpu);
					streamingManager.addCopyCompletionEvent(requester, FeedbackAnalizerSubPass::readbackTextureReady<Executor>);
				}, executor->sharedResources->graphicsEngine.frameIndex);
			}));
			
			update2(commandLists, numThreads);
		}

		static void addBarrier(FeedbackAnalizerSubPass& renderSubPass, ID3D12GraphicsCommandList* lastCommandList);
	};
};