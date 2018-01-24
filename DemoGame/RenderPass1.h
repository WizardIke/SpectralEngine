#pragma once
#include <RenderPass.h>
#include <RenderSubPass.h>
#include <MainCamera.h>
#include <ReflectionCamera.h>
#include <d3d12.h>
#include <tuple>
#include <Range.h>
#include <FeedbackAnalizer.h>

class RenderPass1
{
	constexpr static unsigned int virtualTextureFeedbackSubPassIndex = 0u;
	constexpr static unsigned int renderToTextureSubPassIndex = 1u;
	constexpr static unsigned int colorSubPassIndex = 2u;
	using RenderToTextureSubPass1 = RenderSubPass<ReflectionCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u>;
	using RenderToTextureSubPassGroup1 = RenderSubPassGroup<RenderToTextureSubPass1>;
	using ColorSubPass1 = RenderMainSubPass<MainCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
		std::tuple<std::integral_constant<unsigned int, renderToTextureSubPassIndex>>,
		std::tuple<std::integral_constant<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE>>, 2u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT>;
	using RenderPass11 = RenderPass<FeedbackAnalizerSubPass, RenderToTextureSubPassGroup1, ColorSubPass1>;

	RenderPass11 data;
public:
	using RenderToTextureSubPass = RenderToTextureSubPass1;

	RenderPass1() {}
	RenderPass1(SharedResources& sharedResources, MainCamera& mainCamera, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpu, uint8_t* constantBufferCpu, float feedbackFieldOfView) : data(sharedResources)
	{
		new(&std::get<virtualTextureFeedbackSubPassIndex>(data.subPasses)) FeedbackAnalizerSubPass{ sharedResources, mainCamera.width() / 2u, mainCamera.height() / 2u, this->data,
			constantBufferGpu, constantBufferCpu, feedbackFieldOfView };
	}

	class Local
	{
		RenderPass11::ThreadLocal data;
	public:
		Local(SharedResources& sharedResources) : data(sharedResources) {}

		class ColorSubPass
		{
			friend class Local;
			ColorSubPass1::ThreadLocal& data;
			ColorSubPass(RenderPass1::ColorSubPass1::ThreadLocal& data) : data(data) {}
		public:
			ColorSubPass(const ColorSubPass& other) = default;
			ID3D12GraphicsCommandList* opaqueCommandList() { return data.currentData->commandLists[0u]; }
			ID3D12GraphicsCommandList* transparentCommandList() { return data.currentData->commandLists[1u]; }
		};

		ColorSubPass colorSubPass() { return std::get<colorSubPassIndex>(data.subPassesThreadLocal); }

		using RenderToTextureSubPassGroup = RenderToTextureSubPassGroup1::ThreadLocal;
		RenderToTextureSubPassGroup& renderToTextureSubPassGroup() { return std::get<renderToTextureSubPassIndex>(data.subPassesThreadLocal); }
		FeedbackAnalizerSubPass::ThreadLocal& virtualTextureFeedbackSubPass() { return std::get<virtualTextureFeedbackSubPassIndex>(data.subPassesThreadLocal); }

		void update1(BaseExecutor* const executor, RenderPass1& renderPass, unsigned int notFirstThread)
		{
			data.update1(executor, renderPass.data, notFirstThread);
		}

		void update1After(SharedResources& sharedResources, RenderPass1& renderPass, ID3D12RootSignature* rootSignature, unsigned int index)
		{
			data.update1After(sharedResources, renderPass.data, rootSignature, index);
		}
		template<class Executor, class SharedReources>
		void update2(Executor* const executor, SharedReources& sharedResources, RenderPass1& renderPass) { data.update2(executor, sharedResources, renderPass.data); }
		template<class Executor, class SharedReources>
		void update2LastThread(Executor* const executor, SharedReources& sharedResources, RenderPass1& renderPass) { data.update2LastThread(executor, sharedResources, renderPass.data); }
	};

	ColorSubPass1& colorSubPass() { return std::get<colorSubPassIndex>(data.subPasses); }
	using RenderToTextureSubPassGroup = RenderToTextureSubPassGroup1;
	RenderToTextureSubPassGroup& renderToTextureSubPassGroup() { return std::get<renderToTextureSubPassIndex>(data.subPasses); }
	FeedbackAnalizerSubPass& virtualTextureFeedbackSubPass() { return std::get<virtualTextureFeedbackSubPassIndex>(data.subPasses); }

	void update1(D3D12GraphicsEngine& graphicsEngine)
	{
		data.update1(graphicsEngine);
	}

	void updateBarrierCount()
	{
		data.updateBarrierCount();
	}
};