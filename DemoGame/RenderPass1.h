#pragma once
#include <RenderPass.h>
#include <RenderSubPass.h>
#include <MainCamera.h>
#include <d3d12.h>
#include <tuple>

class RenderPass1
{
	using ColorRenderSubPass1 = RenderSubPass<MainCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, std::tuple<std::integral_constant<unsigned int, 1u>>, std::tuple<std::integral_constant<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATE_RENDER_TARGET>>, 2u>;
	using PresentSubPass1 = RenderSubPass<MainCamera, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, std::tuple<std::integral_constant<unsigned int, 0u>>, std::tuple<std::integral_constant<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATE_PRESENT>>, 0u>;
	using RenderPass11 = RenderPass<ColorRenderSubPass1, PresentSubPass1>;
	constexpr static unsigned int colorSubPassIndex = 0u;

	RenderPass11 data;
public:
	RenderPass1(BaseExecutor* const executor) : data(executor) {}

	class Local
	{
		RenderPass1::RenderPass11::ThreadLocal data;
	public:
		Local(BaseExecutor* const executor) : data(executor) {}

		class ColorSubPass
		{
			friend class Local;
			RenderPass1::ColorRenderSubPass1::ThreadLocal& data;
			ColorSubPass(RenderPass1::ColorRenderSubPass1::ThreadLocal& data) : data(data) {}
			
		public:
			ColorSubPass(const ColorSubPass& other) = default;
			ID3D12GraphicsCommandList* opaqueCommandList() { return data.currentData->commandLists[0u]; }
			ID3D12GraphicsCommandList* transparentCommandList() { return data.currentData->commandLists[1u]; }
		};

		ColorSubPass colorSubPass() { return std::get<colorSubPassIndex>(data.subPassesThreadLocal); }

		void update1(BaseExecutor* const executor, RenderPass1& renderPass, unsigned int notFirstThread)
		{
			data.update1(executor, renderPass.data, notFirstThread);
		}

		void update1After(BaseExecutor* const executor, RenderPass1& renderPass, ID3D12RootSignature* rootSignature, unsigned int index)
		{
			data.update1After(executor, renderPass.data, rootSignature, index);
		}
		void update2(BaseExecutor* const executor, RenderPass1& renderPass, unsigned int index) { data.update2(executor, renderPass.data, index); }
		void update2LastThread(BaseExecutor* const executor, RenderPass1& renderPass, unsigned int index) { data.update2LastThread(executor, renderPass.data, index); }
	};

	class ColorSubPass
	{
		friend class RenderPass1;
		RenderPass1& parent;
		ColorSubPass(RenderPass1& parent) : parent(parent) {}
	public:
		ColorSubPass(const ColorSubPass& other) = default;
		void addCamera(BaseExecutor* executor, std::remove_reference_t<decltype(*RenderPass1::ColorRenderSubPass1::cameras[0u])>* camera)
		{
			parent.data.addCamera<colorSubPassIndex, std::remove_reference_t<decltype(*RenderPass1::ColorRenderSubPass1::cameras[0u])>>(executor, camera);
		}

		void removeCamera(BaseExecutor* executor, std::remove_reference_t<decltype(*RenderPass1::ColorRenderSubPass1::cameras[0u])>* camera)
		{
			parent.data.removeCamera<colorSubPassIndex, std::remove_reference_t<decltype(*RenderPass1::ColorRenderSubPass1::cameras[0u])>>(executor, camera);
		}
	};

	ColorSubPass colorSubPass() { return ColorSubPass(*this); }

	void update1(D3D12GraphicsEngine& graphicsEngine)
	{
		data.update1(graphicsEngine);
	}
};