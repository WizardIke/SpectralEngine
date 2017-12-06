#pragma once
#include <d3d12.h>
#include <memory>
#include "BaseExecutor.h"
#include "SharedResources.h"
#include "for_each.h"
#include "Frustum.h"

template<class... RenderSubPass_t>
class RenderPass
{
	std::tuple<RenderSubPass_t...> subPasses;
	std::unique_ptr<ID3D12CommandList*[]> commandLists;//size = subPassCount * threadCount * 2
	unsigned int commandListCount = 0u;
	BaseExecutor* firstExector;
	unsigned int maxBarriers = 0u;
	std::unique_ptr<D3D12_RESOURCE_BARRIER[]> barriers = nullptr;//size = maxDependences * maxCameras * 2
public:
	RenderPass(BaseExecutor* const executor) : commandLists(new ID3D12CommandList*[sizeof...(RenderSubPass_t) * executor->sharedResources->maxThreads * 2u]) {}

	template<unsigned int subPassIndex, class Camera>
	void addCamera(BaseExecutor* const executor, Camera* const camera)
	{
		auto& cameras = std::get<subPassIndex>(subPasses).cameras;
		std::lock_guard<decltype(executor->sharedResources->syncMutex)> lock(executor->sharedResources->syncMutex);
		cameras.push_back(camera);

		//grow barriers if needed
		unsigned int maxDependences = 0u, maxCameras = 0u;
		for_each(subPasses, [&maxDependences, &maxCameras](auto& subPass)
		{
			if (std::tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value > maxDependences)
			{
				maxDependences = (unsigned int)std::tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value;
			}
			if (subPass.cameraCount() > maxCameras)
			{
				maxCameras = subPass.cameraCount();
			}
		});

		auto newSize = maxDependences * maxCameras * 2u;
		if (newSize > maxBarriers)
		{
			barriers.reset(new D3D12_RESOURCE_BARRIER[newSize]);
			maxBarriers = newSize;
		}
	}

	template<unsigned int subPassIndex, class Camera>
	void removeCamera(BaseExecutor* const executor, Camera* const camera)
	{
		auto& cameras = std::get<subPassIndex>(subPasses).cameras;
		std::lock_guard<decltype(executor->sharedResources->syncMutex)> lock(executor->sharedResources->syncMutex);
		auto cam = std::find(cameras.begin(), cameras.end(), camera);
		std::swap(*cam, *(cameras.end() - 1u));
		cameras.pop_back();
	}

	class ThreadLocal
	{
	public:
		std::tuple<typename RenderSubPass_t::ThreadLocal...> subPassesThreadLocal;
	private:

		template<unsigned int start, class SubPasses>
		constexpr static std::enable_if_t<!(std::tuple_element_t<start, SubPasses>::empty), unsigned int> getLastCameraSubPass()
		{
			return start;
		}

		template<unsigned int start, class SubPasses>
		constexpr static std::enable_if_t<std::tuple_element_t<start, SubPasses>::empty, unsigned int> getLastCameraSubPass()
		{
			return getLastCameraSubPass<start != 0u ? start - 1u : std::tuple_size<SubPasses>::value - 1u, SubPasses>();
		}

		template<unsigned int start, class SubPasses>
		constexpr static std::enable_if_t<!(std::tuple_element_t<start, SubPasses>::empty), unsigned int> getNextCameraSubPass()
		{
			return start;
		}

		template<unsigned int start, class SubPasses>
		constexpr static std::enable_if_t<std::tuple_element_t<start, SubPasses>::empty, unsigned int> getNextCameraSubPass()
		{
			return getNextCameraSubPass<start != std::tuple_size<SubPasses>::value - 1 ? start + 1u : 0u, SubPasses>();
		}


		template<unsigned int startIndex, unsigned int endIndex, unsigned int currentSubPassIndex, unsigned int nextSubPassIndex>
		static std::enable_if_t<!(startIndex < endIndex), void> addEndOnlyBarriers(BaseExecutor* executor, const Frustum& mainFrustum,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}

		template<unsigned int startIndex, unsigned int endIndex, unsigned int currentSubPassIndex, unsigned int nextSubPassIndex>
		static std::enable_if_t < startIndex < endIndex, void> addEndOnlyBarriers(BaseExecutor* executor, const Frustum& mainFrustum,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextSubPassIndex, SubPasses>;
			constexpr auto dependencyIndex = std::tuple_element_t<startIndex, NextSubPass::Dependencies>::value;
			constexpr auto cameraSubPassIndex = getLastCameraSubPass<startIndex, SubPasses>();
			if (dependencyIndex != currentSubPassIndex)
			{
				addEndOnlyBarrier<std::tuple_element_t<dependencyIndex, SubPasses>, std::tuple_element_t<cameraSubPassIndex, SubPasses>>(executor,
					mainFrustum, std::get<cameraSubPassIndex>(subPasses), barriers, barrierCount);
				addEndOnlyBarriers<startIndex + 1u, endIndex, currentSubPassIndex, nextSubPassIndex>(executor, mainFrustum, subPasses, barriers, barrierCount);
			}
		}

		template<class DependencySubPass, class NextSubPass, class CameraSubPass>
		static void addEndOnlyBarrier(BaseExecutor* executor, const Frustum& mainFrustum, const CameraSubPass& dependencySubPass, D3D12_RESOURCE_BARRIER* barriers,
			unsigned int& barrierCount)
		{
			for (auto camera : dependencySubPass.cameras)
			{
				if (camera->isInView(mainFrustum))
				{
					barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
					barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barriers[barrierCount].Transition.pResource = camera->getImage(executor);
					barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[barrierCount].Transition.StateBefore = DependencySubPass::state;
					barriers[barrierCount].Transition.StateAfter = NextSubPass::state;
					++barrierCount;
				}
			}
		}

		template<class Dependencies, unsigned int index, unsigned int start = 0u, unsigned int end = std::tuple_size<Dependencies>::value>
		constexpr static std::enable_if_t < start < end, unsigned int> findDependency()
		{
			return std::tuple_element_t<start, Dependencies>::value == index ? start : findDependency<Dependencies, index, start + 1u, end>();
		}

		template<class Dependencies, unsigned int index, unsigned int start = 0u, unsigned int end = std::tuple_size<Dependencies>::value>
		constexpr static std::enable_if_t<!(start < end), unsigned int> findDependency()
		{
			return end;
		}


		template<unsigned int previousSubPassIndex, unsigned int startIndex>
		static std::enable_if_t<!(startIndex != previousSubPassIndex), void> addBeginBarriers(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses, Frustum& mainFrustum,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}

		template<unsigned int previousSubPassIndex, unsigned int startIndex>
		static std::enable_if_t<startIndex != previousSubPassIndex, void> addBeginBarriers(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			Frustum& mainFrustum, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr auto dependencyIndex = findDependency<typename std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::Dependencies, previousSubPassIndex>();
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			if (dependencyIndex != subPassCount)
			{
				constexpr unsigned int lastCameraSubPass = getLastCameraSubPass<previousSubPassIndex, std::tuple<RenderSubPass_t...>>();
				for (auto camera : std::get<lastCameraSubPass>(subPasses).cameras)
				{
					if (camera->isInView(mainFrustum))
					{
						if (startIndex == (previousSubPassIndex + 1u) % subPassCount)
						{
							barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
						}
						else
						{
							barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
						}
						barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						barriers[barrierCount].Transition.pResource = camera->getImage(executor);
						barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						barriers[barrierCount].Transition.StateBefore = std::tuple_element_t<previousSubPassIndex, std::tuple<RenderSubPass_t...>>::state;
						barriers[barrierCount].Transition.StateAfter = std::tuple_element_t<dependencyIndex, typename std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::DependencyStates>::value;
						++barrierCount;
					}
				}
			}
			else
			{
				addBeginBarriers<previousSubPassIndex, (startIndex + 1u) % subPassCount>(executor, subPasses, mainFrustum, barriers, barrierCount);
			}
		}

		/*
			Adds all resource barriers for one subPass
		*/
		template<unsigned int previousIndex, unsigned int nextIndex>
		static void addBarriers(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int& barrierCount)
		{
			addBeginBarriers<previousIndex, nextIndex>(executor, renderPass.subPasses, executor->sharedResources->mainFrustum, renderPass.barriers.get(), barrierCount);
			using NextSubPass = std::tuple_element_t<nextIndex, decltype(renderPass.subPasses)>;
			constexpr auto nextSubPassDependencyCount = std::tuple_size<typename NextSubPass::Dependencies>::value;
			addEndOnlyBarriers<0u, nextSubPassDependencyCount, previousIndex, nextIndex>(executor,
				executor->sharedResources->mainFrustum, renderPass.subPasses, renderPass.barriers.get(), barrierCount);
		}
	public:
		ThreadLocal(BaseExecutor* const executor) : subPassesThreadLocal((sizeof(typename RenderSubPass_t::ThreadLocal), executor)...) {}

		void update1After(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature, unsigned int notFirstThread)
		{
			if (notFirstThread == 0u)
			{
				renderPass.commandListCount = 0u;
				renderPass.firstExector = executor;
				update1After<0u, sizeof...(RenderSubPass_t), true>(executor, renderPass, rootSignature);
			}
			else
			{
				update1After<0u, sizeof...(RenderSubPass_t), false>(executor, renderPass, rootSignature);
			}
		}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<!(start != end), void> update1After(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature) {}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<start != end, void>
			update1After(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature)
		{
			update1AfterHelper<start, end, firstThread>(executor, renderPass, rootSignature);
		}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<!std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::empty, void>
			update1AfterHelper(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature)
		{
			auto& subPassLocal = std::get<start>(subPassesThreadLocal);
			auto& subPass = std::get<start>(renderPass.subPasses);
			if (subPass.isInView(executor->sharedResources->mainFrustum))
			{
				if (firstThread)
				{
					unsigned int barriarCount = 0u;
					addBarriers<start == 0u ? end - 1u : start - 1u, start>(executor, renderPass, barriarCount);
					subPassLocal.update1AfterFirstThread(executor, subPass, rootSignature, barriarCount, renderPass.barriers.get());
				}
				else
				{
					subPassLocal.update1After(executor, subPass, rootSignature);
				}
			}
			update1After<start + 1u, end, firstThread>(executor, renderPass, rootSignature);
		}


		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::empty, void>
			update1AfterHelper(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature)
		{
			update1After<start + 1u, end, firstThread>(executor, renderPass, rootSignature);
		}

		void update2(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int index)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			if (executor == renderPass.firstExector) 
			{
				update2<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get());
			}
			else
			{
				update2<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get() + renderPass.commandListCount);
				renderPass.commandListCount += std::tuple_element_t<0u, std::tuple<RenderSubPass_t...>>::commandListsPerFrame;
			}
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<!(start < end), void>
			update2(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists) {}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start < end, void>
			update2(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists)
		{
			update2Helper<start, end>(executor, renderPass, commandLists);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<!std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::empty, void>
			update2Helper(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists)
		{
			auto& subPassLocal = std::get<start>(subPassesThreadLocal);
			auto& subPass = std::get<start>(renderPass.subPasses);
			if (subPass.isInView(executor->sharedResources->mainFrustum))
			{
				subPassLocal.update2(commandLists);
				commandLists += (1u + executor->sharedResources->maxPrimaryThreads +
					executor->sharedResources->numPrimaryJobExeThreads) * std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::commandListsPerFrame;
			}
			update2<start + 1u, end>(executor, renderPass, commandLists);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::empty, void>
			update2Helper(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists)
		{
			update2<start + 1u, end>(executor, renderPass, commandLists);
		}

		void update2LastThread(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int index)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			ID3D12CommandList** newCommandLists;
			if (executor == renderPass.firstExector)
			{
				newCommandLists = update2LastThread<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get());
			}
			else
			{
				newCommandLists = update2LastThread<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get() + renderPass.commandListCount);
				renderPass.commandListCount += std::tuple_element_t<0u, std::tuple<RenderSubPass_t...>>::commandListsPerFrame;
			}
			unsigned int commandListCount = static_cast<unsigned int>(newCommandLists - renderPass.commandLists.get());
			executor->sharedResources->graphicsEngine.present(executor->sharedResources->window, renderPass.commandLists.get(), commandListCount);
		}


		template<unsigned int start, unsigned int end>
		std::enable_if_t<!(start < end), ID3D12CommandList**>
			update2LastThread(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists)
		{
			return commandLists;
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start < end, ID3D12CommandList**>
			update2LastThread(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists)
		{
			return update2LastThreadHelper<start, end>(executor, renderPass, commandLists);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::empty, ID3D12CommandList**>
			update2LastThreadHelper(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists)
		{
			return update2LastThread<start + 1u, end>(executor, renderPass, commandLists);
		}

		template<unsigned int start, unsigned int subPassCount>
		std::enable_if_t<!std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::empty, ID3D12CommandList**>
			update2LastThreadHelper(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists)
		{
			constexpr auto nextSubPassIndex = getNextCameraSubPass<(start + 1u) % subPassCount, std::tuple<RenderSubPass_t...>>();
			update2LastThreadEmptySubpasses<(start + 1u) % subPassCount, subPassCount>(executor, renderPass);

			auto& subPassLocal = std::get<start>(subPassesThreadLocal);
			auto& subPass = std::get<start>(renderPass.subPasses);
			if (subPass.isInView(executor->sharedResources->mainFrustum))
			{
				subPassLocal.update2(commandLists);
				commandLists += (1u + executor->sharedResources->maxPrimaryThreads +
					executor->sharedResources->numPrimaryJobExeThreads) * std::tuple_element_t<start, std::tuple<RenderSubPass_t...>>::commandListsPerFrame;
			}
			if (nextSubPassIndex > start)
			{
				return update2LastThread<nextSubPassIndex, subPassCount>(executor, renderPass, commandLists);
			}
			return commandLists;
		}

		template<unsigned int nextSubPassIndex, unsigned int subPassCount>
		std::enable_if_t<!std::tuple_element_t<nextSubPassIndex, std::tuple<RenderSubPass_t...>>::empty, void>
			update2LastThreadEmptySubpasses(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass) {}

		template<unsigned int subPassIndex, unsigned int subPassCount>
		std::enable_if_t<std::tuple_element_t<subPassIndex, std::tuple<RenderSubPass_t...>>::empty, void>
			update2LastThreadEmptySubpasses(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass)
		{
			unsigned int barriarCount = 0u;
			constexpr auto nextSubPassIndex = (subPassIndex + 1u) % subPassCount;
			addBarriers<subPassIndex == 0u ? subPassCount - 1u : subPassIndex - 1u, subPassIndex>(executor, renderPass, barriarCount);
			auto& subPass = std::get<subPassIndex>(renderPass.subPasses);
			constexpr unsigned int lastCameraSubPassIndex = getLastCameraSubPass<subPassIndex, std::tuple<RenderSubPass_t...>>();
			auto& latCameraSubPassLocal = std::get<lastCameraSubPassIndex>(subPassesThreadLocal);
			subPass.update2LastThread(latCameraSubPassLocal.lastCommandList(), barriarCount, renderPass.barriers.get());

			update2LastThreadEmptySubpasses<nextSubPassIndex, subPassCount>(executor, renderPass);
		}
	};

	void update1(D3D12GraphicsEngine& graphicsEngine)
	{
		graphicsEngine.waitForPreviousFrame();
	}
};