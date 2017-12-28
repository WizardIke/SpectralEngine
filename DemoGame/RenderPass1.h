#pragma once
#include <RenderPass.h>
#include <RenderSubPass.h>
#include <MainCamera.h>
#include <Camera.h>
#include <d3d12.h>
#include <tuple>
#include <Iterable.h>

class RenderPass1
{
	constexpr static unsigned int renderToTextureSubPassIndex = 0u;
	constexpr static unsigned int colorSubPassIndex = 1u;
	using RenderToTextureSubPass1 = RenderSubPass<Camera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<>, std::tuple<>, 1u>;
	using RenderToTextureSubPassGroup1 = RenderSubPassGroup<RenderToTextureSubPass1>;
	using ColorSubPass1 = RenderSubPass<MainCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, 
		std::tuple<std::integral_constant<unsigned int, renderToTextureSubPassIndex>>,
		std::tuple<std::integral_constant<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE>>, 2u, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, true>;
	using RenderPass11 = RenderPass<RenderToTextureSubPassGroup1, ColorSubPass1>;

	RenderPass11 data;
public:
	using RenderToTextureSubPass = RenderToTextureSubPass1;
	RenderPass1(BaseExecutor* const executor) : data(executor) {}

	class Local
	{
		RenderPass11::ThreadLocal data;
	public:
		Local(BaseExecutor* const executor) : data(executor) {}

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

		void update1(BaseExecutor* const executor, RenderPass1& renderPass, unsigned int notFirstThread)
		{
			data.update1(executor, renderPass.data, notFirstThread);
		}

		void update1After(BaseExecutor* const executor, RenderPass1& renderPass, ID3D12RootSignature* rootSignature, unsigned int index)
		{
			data.update1After(executor, renderPass.data, rootSignature, index);
		}
		void update2(BaseExecutor* const executor, RenderPass1& renderPass) { data.update2(executor, renderPass.data); }
		void update2LastThread(BaseExecutor* const executor, RenderPass1& renderPass) { data.update2LastThread(executor, renderPass.data); }
	};

	ColorSubPass1& colorSubPass() { return std::get<colorSubPassIndex>(data.subPasses); }
	using RenderToTextureSubPassGroup = RenderToTextureSubPassGroup1;
	RenderToTextureSubPassGroup& renderToTextureSubPassGroup() { return std::get<renderToTextureSubPassIndex>(data.subPasses); }

	void update1(D3D12GraphicsEngine& graphicsEngine)
	{
		data.update1(graphicsEngine);
	}

	void updateBarrierCount()
	{
		data.updateBarrierCount();
	}
};