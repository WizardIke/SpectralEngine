#pragma once
#include <RenderPass.h>
#include <RenderSubPass.h>
#include <MainCamera.h>
#include <ReflectionCamera.h>
#include <d3d12.h>
#include <tuple>
#include <Range.h>
#include <VirtualFeedbackSubPass.h>
class GlobalResources;
class ThreadResources;

class RenderPass1
{
	constexpr static unsigned int virtualTextureFeedbackSubPassIndex = 0u;
	constexpr static unsigned int renderToTextureSubPassIndex = 1u;
	constexpr static unsigned int colorSubPassIndex = 2u;
public:
	using RenderToTextureSubPass = RenderSubPass<ReflectionCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u>;
	using ColorSubPass = RenderMainSubPass<MainCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
		std::tuple<std::integral_constant<unsigned int, renderToTextureSubPassIndex>>,
		std::tuple<std::integral_constant<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE>>, 2u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT>;
private:
	using RenderPass11 = RenderPass<VirtualFeedbackSubPass, RenderToTextureSubPass, ColorSubPass>;

	RenderPass11 data;
public:
	RenderPass1() {}
	RenderPass1(GlobalResources& globalResources, Transform& mainCameraTransform, uint32_t virtualTextureFeedbackWidth, uint32_t virtualTextureFeedbackHeight,
		D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpu, unsigned char*& constantBufferCpu, float feedbackFieldOfView);

	class Local
	{
		RenderPass11::ThreadLocal data;
	public:
		Local(GraphicsEngine& graphicsEngine) : data(graphicsEngine) {}

		class ColorSubPass
		{
			friend class Local;
			RenderPass1::ColorSubPass::ThreadLocal& data;
			ColorSubPass(RenderPass1::ColorSubPass::ThreadLocal& data) : data(data) {}
		public:
			ColorSubPass(const ColorSubPass& other) = default;
			ID3D12GraphicsCommandList* opaqueCommandList() { return data.currentData->commandLists[0u]; }
			ID3D12GraphicsCommandList* transparentCommandList() { return data.currentData->commandLists[1u]; }
		};

		ColorSubPass colorSubPass() { return std::get<colorSubPassIndex>(data.subPassesThreadLocal); }

		RenderToTextureSubPass::ThreadLocal& renderToTextureSubPass() { return std::get<renderToTextureSubPassIndex>(data.subPassesThreadLocal); }
		VirtualFeedbackSubPass::ThreadLocal& virtualTextureFeedbackSubPass() { return std::get<virtualTextureFeedbackSubPassIndex>(data.subPassesThreadLocal); }

		void update1(ThreadResources& threadResources, GlobalResources& globalResources, RenderPass1& renderPass, bool firstThread)
		{
			data.update1(threadResources, globalResources, renderPass.data, firstThread);
		}

		void update1After(GraphicsEngine& graphicsEngine, RenderPass1& renderPass, ID3D12RootSignature* rootSignature, bool firstThread)
		{
			data.update1After(graphicsEngine, renderPass.data, rootSignature, firstThread);
		}

		void update2(ThreadResources& threadResources, GlobalResources& globalResources, RenderPass1& renderPass, unsigned int threadCount) { data.update2(threadResources, globalResources, renderPass.data, threadCount); }
		
		void present(unsigned int primaryThreadCount, GraphicsEngine& graphicsEngine, Window& window, RenderPass1& renderPass) { data.present(primaryThreadCount, graphicsEngine, window, renderPass.data); }
	};

	ColorSubPass& colorSubPass() { return std::get<colorSubPassIndex>(data.subPasses()); }
	RenderToTextureSubPass& renderToTextureSubPass() { return std::get<renderToTextureSubPassIndex>(data.subPasses()); }
	VirtualFeedbackSubPass& virtualTextureFeedbackSubPass() { return std::get<virtualTextureFeedbackSubPassIndex>(data.subPasses()); }

	void addMessage(RenderPassMessage& message, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		data.addMessage(message, threadResources, globalResources);
	}
};