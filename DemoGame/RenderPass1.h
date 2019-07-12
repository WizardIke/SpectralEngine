#pragma once

#include <RenderPass.h>
#include <RenderSubPass.h>
#include <VirtualFeedbackSubPass.h>
#include <MainCamera.h>
#include <ReflectionCamera.h>
#include <d3d12.h>
#include <Tuple.h>
#include <Range.h>
#include <TaskShedular.h>
#include <Window.h>
class ThreadResources;

class RenderPass1
{
	constexpr static unsigned int virtualTextureFeedbackSubPassIndex = 0u;
	constexpr static unsigned int renderToTextureSubPassIndex = 1u;
	constexpr static unsigned int colorSubPassIndex = 2u;
public:
	using RenderToTextureSubPass = RenderSubPass<ReflectionCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, Tuple<>, Tuple<>, 1u>;
	using ColorSubPass = RenderMainSubPass<MainCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
		Tuple<std::integral_constant<unsigned int, renderToTextureSubPassIndex>>,
		Tuple<std::integral_constant<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE>>, 2u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT>;
private:
	using RenderPass11 = RenderPass<VirtualFeedbackSubPass, RenderToTextureSubPass, ColorSubPass>;

	RenderPass11 data;
public:
	RenderPass1(TaskShedular<ThreadResources>& taskShedular, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager,
		uint32_t virtualTextureFeedbackWidth, uint32_t virtualTextureFeedbackHeight, Transform& mainCameraTransform, float fieldOfView, Window& window);

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpu, unsigned char*& constantBufferCpu);

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

		ColorSubPass colorSubPass()
		{
			using std::get;
			return get<colorSubPassIndex>(data.subPassesThreadLocal);
		}

		RenderToTextureSubPass::ThreadLocal& renderToTextureSubPass()
		{
			using std::get;
			return get<renderToTextureSubPassIndex>(data.subPassesThreadLocal);
		}
		VirtualFeedbackSubPass::ThreadLocal& virtualTextureFeedbackSubPass()
		{
			using std::get;
			return get<virtualTextureFeedbackSubPassIndex>(data.subPassesThreadLocal);
		}

		void update1After(GraphicsEngine& graphicsEngine, RenderPass1& renderPass, ID3D12RootSignature* rootSignature, unsigned int primaryThreadIndex)
		{
			data.update1After(graphicsEngine, renderPass.data, rootSignature, primaryThreadIndex);
		}

		void update2(ThreadResources& threadResources, RenderPass1& renderPass, unsigned int threadCount, unsigned int primaryThreadIndex) { data.update2(threadResources, renderPass.data, threadCount, primaryThreadIndex); }
		
		void present(unsigned int primaryThreadCount, GraphicsEngine& graphicsEngine, Window& window, RenderPass1& renderPass) { data.present(primaryThreadCount, graphicsEngine, window, renderPass.data); }
	};

	ColorSubPass& colorSubPass()
	{
		using std::get;
		return get<colorSubPassIndex>(data.subPasses());
	}
	RenderToTextureSubPass& renderToTextureSubPass()
	{
		using std::get;
		return get<renderToTextureSubPassIndex>(data.subPasses());
	}
	VirtualFeedbackSubPass& virtualTextureFeedbackSubPass()
	{
		using std::get;
		return get<virtualTextureFeedbackSubPassIndex>(data.subPasses());
	}

	void addMessage(RenderPassMessage& message, ThreadResources& threadResources)
	{
		data.addMessage(message, threadResources);
	}

	void addMessageFromBackground(RenderPassMessage& message, TaskShedular<ThreadResources>& taskShedular)
	{
		data.addMessageFromBackground(message, taskShedular);
	}
};