#pragma once
#include <d3d12.h>
#include <memory>
#include "BaseExecutor.h"
#include "SharedResources.h"
#include "for_each.h"
#include "Frustum.h"
#include <utility>

template<class... RenderSubPass_t>
class RenderPass
{
	std::unique_ptr<ID3D12CommandList*[]> commandLists;//size = subPassCount * threadCount * commandListsPerFrame
	unsigned int commandListCount = 0u;
	BaseExecutor* firstExector;
	unsigned int maxBarriers = 0u;
	std::unique_ptr<D3D12_RESOURCE_BARRIER[]> barriers;//size = maxDependences * maxCameras * 2u
public:
	RenderPass(BaseExecutor* const executor)
	{
		unsigned int commandListCount = 0u;
		for_each(subPasses, [&commandListCount](auto& subPass)
		{
			commandListCount += subPass.commandListsPerFrame;
		});
		commandListCount += executor->sharedResources->maxThreads;
		commandLists.reset(new ID3D12CommandList*[commandListCount]);
	}

	std::tuple<RenderSubPass_t...> subPasses;

	void updateBarrierCount()
	{
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

	class ThreadLocal
	{
	public:
		std::tuple<typename RenderSubPass_t::ThreadLocal...> subPassesThreadLocal;
	private:
		template<unsigned int nextIndex>
		static void addEndOnlyBarriers(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers,
			unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? subPassCount - 1u : nextIndex - 1u;
			using ResourceIndices = typename NextSubPass::Dependencies;
			
			addEndOnlyBarriers<nextIndex, 0u, std::tuple_size<ResourceIndices>::value, ResourceIndices>(executor, subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex, unsigned int currentIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentIndex != endIndex, void> addEndOnlyBarriers(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? subPassCount - 1u : nextIndex - 1u;
			constexpr unsigned int index = std::tuple_element_t<currentIndex, ResourceIndices>::value;
			addEndOnlyBarriersOneResource<previousIndex, nextIndex, index>(executor, subPasses, barriers, barrierCount);
			addEndOnlyBarriers<nextIndex, currentIndex + 1u, endIndex, ResourceIndices>(executor, subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex, unsigned int currentIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentIndex == endIndex, void> addEndOnlyBarriers(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}


		template<unsigned int previousIndex, unsigned int nextIndex, unsigned int transitioningResourceIndex>
		static std::enable_if_t<!(previousIndex != nextIndex), void> addEndOnlyBarriersOneResource(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}

		template<unsigned int previousIndex, unsigned int nextIndex, unsigned int transitioningResourceIndex>
		static std::enable_if_t<previousIndex != nextIndex, void> addEndOnlyBarriersOneResource(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			using PreviousSubPass = std::tuple_element_t<previousIndex, SubPasses>;
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependecyCount = std::tuple_size<typename PreviousSubPass::Dependencies>::value;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>();
			if (dependencyIndex != dependecyCount || (previousIndex == transitioningResourceIndex && nextIndex != (previousIndex + 1) % subPassCount))
			{
				addEndOnlyBarriersOneResourceHelper<previousIndex, nextIndex, transitioningResourceIndex, dependencyIndex, dependecyCount>(executor,
					subPasses, barriers, barrierCount);
			}
			else
			{
				addEndOnlyBarriersOneResource<previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u, nextIndex, transitioningResourceIndex>(executor,
					subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int previousIndex, unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependecyCount>
		static std::enable_if_t<dependencyIndex != dependecyCount, void> addEndOnlyBarriersOneResourceHelper(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			using PreviousSubPass = std::tuple_element_t<previousIndex, SubPasses>;
			constexpr auto stateAfterIndex = findDependency<transitioningResourceIndex, typename NextSubPass::Dependencies>();
			addEndOnlyBarriersOneResourceHelper2<std::tuple_element_t<dependencyIndex, typename PreviousSubPass::DependencyStates>::value,
				std::tuple_element_t<stateAfterIndex, typename NextSubPass::DependencyStates>::value, transitioningResourceIndex, nextIndex>(executor,
					subPasses, barriers, barrierCount);
		}

		template<unsigned int previousIndex, unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependecyCount>
		static std::enable_if_t<dependencyIndex == dependecyCount, void> addEndOnlyBarriersOneResourceHelper(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			using PreviousSubPass = std::tuple_element_t<previousIndex, SubPasses>;
			constexpr auto stateAfterIndex = findDependency<transitioningResourceIndex, typename NextSubPass::Dependencies>();
			addEndOnlyBarriersOneResourceHelper2<PreviousSubPass::state,
				std::tuple_element_t<stateAfterIndex, typename NextSubPass::DependencyStates>::value, transitioningResourceIndex, nextIndex>(executor,
					subPasses, barriers, barrierCount);
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int cameraSubPassIndex>
		static void addEndOnlyBarriersOneResourceHelper2(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			if (std::get<cameraSubPassIndex>(subPasses).commandListsPerFrame != 0u)
			{
				addEndOnlyBarrier<stateBefore, stateAfter>(executor, std::get<cameraSubPassIndex>(subPasses), barriers, barrierCount);
			}
			else
			{
				addEndOnlyBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, cameraSubPassIndex != 0u ? cameraSubPassIndex - 1u : subPassCount - 1u>(
					executor, subPasses, barriers, barrierCount);
			}
		}


		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, class CameraSubPass>
		static void addEndOnlyBarrier(BaseExecutor* executor, CameraSubPass& cameraSubPass, D3D12_RESOURCE_BARRIER* barriers,
			unsigned int& barrierCount)
		{
			auto cameras = cameraSubPass.cameras();
			for (auto camera : cameras)
			{
				if (camera->isInView(executor))
				{
					barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
					barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barriers[barrierCount].Transition.pResource = camera->getImage();
					barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[barrierCount].Transition.StateBefore = stateBefore;
					barriers[barrierCount].Transition.StateAfter = stateAfter;
					++barrierCount;
				}
			}
		}

		template<unsigned int index, class Dependencies, unsigned int start = 0u, unsigned int end = std::tuple_size<Dependencies>::value>
		constexpr static std::enable_if_t<start != end, unsigned int> findDependency()
		{
			return std::tuple_element_t<start, Dependencies>::value == index ? start : findDependency<index, Dependencies, start + 1u, end>();
		}

		template<unsigned int index, class Dependencies, unsigned int start = 0u, unsigned int end = std::tuple_size<Dependencies>::value>
		constexpr static std::enable_if_t<start == end, unsigned int> findDependency()
		{
			return end;
		}


		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex>
		static std::enable_if_t<!(startIndex != previousSubPassIndex), void> addBeginBarriersOneResource(BaseExecutor* executor,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex>
		static std::enable_if_t<startIndex != previousSubPassIndex, void> addBeginBarriersOneResource(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using Dependencies = typename std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::Dependencies;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, Dependencies>();
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependencyCount = std::tuple_size<Dependencies>::value;
			addBeginBarriersOneResourceHelper<previousSubPassIndex, startIndex, transitioningResourceIndex, dependencyIndex, dependencyCount>(executor, subPasses,
				barriers, barrierCount);
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex != dependencyCount, void> addBeginBarriersOneResourceHelper(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBefore<transitioningResourceIndex, previousSubPassIndex, dependencyIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = std::tuple_element_t<dependencyIndex, typename std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::DependencyStates>::value;
			if (stateBefore != stateAfter)
			{
				addBeginBarriersOneResourceHelper2<previousSubPassIndex, startIndex, transitioningResourceIndex, stateBefore, stateAfter>(executor, subPasses,
					barriers, barrierCount);
			}
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex,
			D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int lastCameraSubPass = previousSubPassIndex>
		static void addBeginBarriersOneResourceHelper2(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			if (std::get<lastCameraSubPass>(subPasses).commandListsPerFrame != 0u)
			{
				auto cameras = std::get<lastCameraSubPass>(subPasses).cameras();
				for (auto camera : cameras)
				{
					if (camera->isInView(executor))
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
						barriers[barrierCount].Transition.pResource = camera->getImage();
						barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						barriers[barrierCount].Transition.StateBefore = stateBefore;
						barriers[barrierCount].Transition.StateAfter = stateAfter;
						++barrierCount;
					}
				}
			}
			else
			{
				addBeginBarriersOneResourceHelper2<previousSubPassIndex, startIndex, transitioningResourceIndex, stateBefore, stateAfter,
					lastCameraSubPass != 0u ? lastCameraSubPass - 1u : subPassCount - 1u>(executor, subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex == dependencyCount, void> addBeginBarriersOneResourceHelper(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			addBeginBarriersOneResource<previousSubPassIndex, (startIndex + 1u) % subPassCount, transitioningResourceIndex>(executor, subPasses, barriers,
				barrierCount);
		}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex, unsigned int dependencyIndex>
		constexpr static std::enable_if_t<transitioningResourceIndex == previousSubPassIndex, D3D12_RESOURCE_STATES> getStateBefore() noexcept
		{
			return std::tuple_element_t<previousSubPassIndex, std::tuple<RenderSubPass_t...>>::state;
		}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex, unsigned int dependencyIndex>
		constexpr static std::enable_if_t<transitioningResourceIndex != previousSubPassIndex, D3D12_RESOURCE_STATES> getStateBefore() noexcept
		{
			return std::tuple_element_t<dependencyIndex, typename std::tuple_element_t<previousSubPassIndex, std::tuple<RenderSubPass_t...>>::DependencyStates>::value;
		}

		template<unsigned int nextIndex>
		static void addBeginBarriers(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? subPassCount - 1u : nextIndex - 1u;
			using SubPass = std::tuple_element_t<previousIndex, std::tuple<RenderSubPass_t...>>;
			using Dependencies = decltype(std::tuple_cat(std::declval<typename SubPass::Dependencies>(),
				std::declval<std::tuple<std::integral_constant<unsigned int, previousIndex>>>()));
			addBeginBarriers<nextIndex, 0u, std::tuple_size<Dependencies>::value, Dependencies>(executor, subPasses, barriers, barrierCount);
		}

	private:
		template<unsigned int nextSubPassIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex != dependencyIndexEnd, void> addBeginBarriers(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextSubPassIndex == 0u ? subPassCount - 1u : nextSubPassIndex - 1u;
			constexpr auto dependency = std::tuple_element_t<dependencyIndex, Dependencies>::value;
			addBeginBarriersOneResource<previousIndex, nextSubPassIndex, dependency>(executor, subPasses, barriers, barrierCount);
			addBeginBarriers<nextSubPassIndex, dependencyIndex + 1u, dependencyIndexEnd, Dependencies>(executor, subPasses, barriers, barrierCount);
		}

		template<unsigned int nextSubPassIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex == dependencyIndexEnd, void> addBeginBarriers(BaseExecutor* executor, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}
	public:

		/*
			Adds all resource barriers for one subPass
		*/
		template<unsigned int nextIndex>
		static void addBarriers(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int& barrierCount)
		{
			addBeginBarriers<nextIndex>(executor, renderPass.subPasses, renderPass.barriers.get(), barrierCount);
			addEndOnlyBarriers<nextIndex>(executor, renderPass.subPasses, renderPass.barriers.get(), barrierCount);
		}
	public:
		ThreadLocal(BaseExecutor* const executor) : subPassesThreadLocal((sizeof(typename RenderSubPass_t::ThreadLocal), executor)...) {}

		void update1(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int notFirstThread)
		{
			if (notFirstThread == 0u)
			{
				renderPass.firstExector = executor;
				renderPass.commandListCount = 0u;
			}
		}

		void update1After(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature, unsigned int notFirstThread)
		{
			if (notFirstThread == 0u)
			{
				update1After<0u, sizeof...(RenderSubPass_t), true>(executor, renderPass, rootSignature);
			}
			else
			{
				update1After<0u, sizeof...(RenderSubPass_t), false>(executor, renderPass, rootSignature);
			}
		}

		void update2(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			const unsigned int threadCount = (1u + executor->sharedResources->maxPrimaryThreads + executor->sharedResources->numPrimaryJobExeThreads);
			if (executor == renderPass.firstExector)
			{
				update2<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get(), 0u, threadCount);
			}
			else
			{
				++renderPass.commandListCount;
				auto listsAfter = threadCount - renderPass.commandListCount;
				if (listsAfter == 1u)
				{
					update2LastThread<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get(), renderPass.commandListCount, 1u);
				}
				else
				{
					update2<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get(), renderPass.commandListCount, listsAfter);
				}
			}
		}

		void update2LastThread(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass)
		{
			update2(executor, renderPass);
			unsigned int commandListCount = commandListPerThread(renderPass.subPasses) *
				(1u + executor->sharedResources->maxPrimaryThreads + executor->sharedResources->numPrimaryJobExeThreads);
			executor->sharedResources->graphicsEngine.present(executor->sharedResources->window, renderPass.commandLists.get(), commandListCount);
		}

	private:
		unsigned int commandListPerThread(std::tuple<RenderSubPass_t...>& subPasses)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			return commandListPerThread<0u, subPassCount>(subPasses);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start != end, unsigned int> commandListPerThread(std::tuple<RenderSubPass_t...>& subPasses)
		{
			return std::get<start>(subPasses).commandListsPerFrame + commandListPerThread<start + 1u, end>(subPasses);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, unsigned int> commandListPerThread(std::tuple<RenderSubPass_t...>& subPasses)
		{
			return 0u;
		}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<!(start != end), void> update1After(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature) {}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<start != end, void>
			update1After(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature)
		{
			const auto commandListsPerFrame = std::get<start>(renderPass.subPasses).commandListsPerFrame;
			if (commandListsPerFrame != 0u)
			{
				auto& subPassLocal = std::get<start>(subPassesThreadLocal);
				auto& subPass = std::get<start>(renderPass.subPasses);
				if (subPass.isInView(executor))
				{
					if (firstThread)
					{
						unsigned int barriarCount = 0u;
						addBarriers<start>(executor, renderPass, barriarCount);
						subPassLocal.update1AfterFirstThread(executor, subPass, rootSignature, barriarCount, renderPass.barriers.get());
					}
					else
					{
						subPassLocal.update1After(executor, subPass, rootSignature);
					}
				}
			}
			update1After<start + 1u, end, firstThread>(executor, renderPass, rootSignature);
		}
	
		template<unsigned int start, unsigned int end>
		std::enable_if_t<!(start < end), void>
			update2(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int listsBefore, unsigned int listsAfter) {}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start < end, void>
			update2(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int listsBefore, unsigned int listsAfter)
		{
			const auto commandListsPerFrame = std::get<start>(renderPass.subPasses).commandListsPerFrame;
			if (commandListsPerFrame != 0u)
			{
				auto& subPassLocal = std::get<start>(subPassesThreadLocal);
				auto& subPass = std::get<start>(renderPass.subPasses);
				if (subPass.isInView(executor))
				{
					commandLists += listsBefore * commandListsPerFrame;
					subPassLocal.update2(commandLists);
					commandLists += listsAfter * commandListsPerFrame;
				}
			}
			update2<start + 1u, end>(executor, renderPass, commandLists, listsBefore, listsAfter);
		}


		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, void>
			update2LastThread(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int listsBefore, unsigned int listsAfter) {}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start != end, void>
			update2LastThread(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int listsBefore, unsigned int listsAfter)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			const auto commandListsPerFrame = std::get<start>(renderPass.subPasses).commandListsPerFrame;
			if (commandListsPerFrame != 0u)
			{
				constexpr auto nextIndex = (start + 1u) % subPassCount;
				update2LastThreadEmptySubpasses<nextIndex, subPassCount>(executor, renderPass);

				auto& subPass = std::get<start>(renderPass.subPasses);
				if (subPass.isInView(executor))
				{
					auto& subPassLocal = std::get<start>(subPassesThreadLocal);
					commandLists += listsBefore * commandListsPerFrame;
					subPassLocal.update2(commandLists);
					commandLists += listsAfter * commandListsPerFrame;
				}
				update2LastThreadOnNextCameraSubPass<nextIndex, end>(executor, renderPass, commandLists, listsBefore, listsAfter);
			}
			else
			{
				update2LastThread<start + 1u, end>(executor, renderPass, commandLists, listsBefore, listsAfter);
			}
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start != end, void> update2LastThreadOnNextCameraSubPass(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass,
			ID3D12CommandList** commandLists, unsigned int listsBefore, unsigned int listsAfter)
		{
			const auto commandListsPerFrame = std::get<start>(renderPass.subPasses).commandListsPerFrame;
			if (commandListsPerFrame != 0u)
			{
				update2LastThread<start, end>(executor, renderPass, commandLists, listsBefore, listsAfter);
			}
			else
			{
				update2LastThreadOnNextCameraSubPass<start + 1u, end>(executor, renderPass, commandLists, listsBefore, listsAfter);
			}
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, void> update2LastThreadOnNextCameraSubPass(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass,
			ID3D12CommandList** commandLists, unsigned int listsBefore, unsigned int listsAfter) {}

		template<unsigned int subPassIndex, unsigned int subPassCount>
		void update2LastThreadEmptySubpasses(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass)
		{
			const auto commandListsPerFrame = std::get<subPassIndex>(renderPass.subPasses).commandListsPerFrame;
			if (commandListsPerFrame == 0u)
			{
				unsigned int barrierCount = 0u;
				addBarriers<subPassIndex>(executor, renderPass, barrierCount);
				auto& subPass = std::get<subPassIndex>(renderPass.subPasses);
				update2LastThreadEmptySubpassesHelper<subPassIndex, subPassIndex != 0u ? subPassIndex - 1u : subPassCount - 1u>(executor, renderPass, barrierCount);
			}
		}

		template<unsigned int subPassIndex, unsigned int lastCameraSubPassIndex>
		void update2LastThreadEmptySubpassesHelper(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			if (std::get<lastCameraSubPassIndex>(renderPass.subPasses).commandListsPerFrame  != 0u)
			{
				auto& latCameraSubPassLocal = std::get<lastCameraSubPassIndex>(subPassesThreadLocal);
				latCameraSubPassLocal.lastCommandList()->ResourceBarrier(barrierCount, renderPass.barriers.get());
				constexpr auto nextSubPassIndex = (subPassIndex + 1u) % subPassCount;
				update2LastThreadEmptySubpasses<nextSubPassIndex, subPassCount>(executor, renderPass);
			}
			else
			{
				update2LastThreadEmptySubpassesHelper<subPassIndex, lastCameraSubPassIndex != 0u ? lastCameraSubPassIndex - 1u : subPassCount - 1u>(executor, renderPass,
					barrierCount);
			}
		}
	};

	void update1(D3D12GraphicsEngine& graphicsEngine)
	{
		graphicsEngine.waitForPreviousFrame();
	}
};