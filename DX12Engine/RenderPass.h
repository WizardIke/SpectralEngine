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
	std::unique_ptr<D3D12_RESOURCE_BARRIER[]> barriers;//size = maxDependences * 2u * maxCameras * maxCommandListsPerFrame
public:
	RenderPass() {}
	RenderPass(SharedResources& sharedResources)
	{
		unsigned int commandListCount = 0u;
		for_each(subPasses, [&commandListCount](auto& subPass)
		{
			commandListCount += subPass.commandListsPerFrame;
		});
		commandListCount += sharedResources.maxThreads;
		commandLists.reset(new ID3D12CommandList*[commandListCount]);
	}

	std::tuple<RenderSubPass_t...> subPasses;

	void updateBarrierCount()
	{
		//grow barriers if needed
		unsigned int maxDependences = 0u, maxCameras = 0u, maxCommandListsPerFrame = 0u;
		for_each(subPasses, [&maxDependences, &maxCameras, &maxCommandListsPerFrame](auto& subPass)
		{
			if (std::tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value > maxDependences)
			{
				maxDependences = (unsigned int)std::tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value;
			}
			if (subPass.cameraCount() > maxCameras)
			{
				maxCameras = subPass.cameraCount();
			}
			if (subPass.commandListsPerFrame > maxCommandListsPerFrame)
			{
				maxCommandListsPerFrame = subPass.commandListsPerFrame;
			}
		});

		auto newSize = maxDependences * maxCameras * maxCommandListsPerFrame * 2u;
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

		template<unsigned int transitioningResourceIndex, class NextSubPass>
		constexpr static D3D12_RESOURCE_STATES getStateAfterEndBarriars()
		{
			return getStateAfterEndBarriarsHelper<NextSubPass, findDependency<transitioningResourceIndex, typename NextSubPass::Dependencies>()>();
		}

		template<class NextSubPass, unsigned int stateAfterIndex>
		constexpr static std::enable_if_t<stateAfterIndex == std::tuple_size<typename NextSubPass::Dependencies>::value, D3D12_RESOURCE_STATES>
			getStateAfterEndBarriarsHelper()
		{
			return NextSubPass::state;
		}

		template<class NextSubPass, unsigned int stateAfterIndex>
		constexpr static std::enable_if_t<stateAfterIndex != std::tuple_size<typename NextSubPass::Dependencies>::value, D3D12_RESOURCE_STATES>
			getStateAfterEndBarriarsHelper()
		{
			return std::tuple_element_t<stateAfterIndex, typename NextSubPass::DependencyStates>::value;
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex>
		static void addBeginBarriersOneResource(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPass = std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>;
			using Dependencies = typename SubPass::Dependencies;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, Dependencies>();
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependencyCount = std::tuple_size<Dependencies>::value;
			if (std::get<startIndex>(subPasses).isInView(sharedResources) && (dependencyIndex != dependencyCount || transitioningResourceIndex == startIndex))
			{
				addBeginBarriersOneResourceHelper<previousSubPassIndex, startIndex, transitioningResourceIndex, dependencyIndex, dependencyCount>(sharedResources, subPasses,
					barriers, barrierCount);
			}
			else if (startIndex != previousSubPassIndex)
			{
				addBeginBarriersOneResource<previousSubPassIndex, startIndex == subPassCount - 1u ? 0u : startIndex + 1u, transitioningResourceIndex>(sharedResources,
					subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex != dependencyCount, void> addBeginBarriersOneResourceHelper(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousSubPassIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = std::tuple_element_t<dependencyIndex, typename std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::DependencyStates>::value;
			if (stateBefore != stateAfter)
			{
				constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
				addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, startIndex, (previousSubPassIndex + 1u) % subPassCount>(sharedResources,
					subPasses, barriers, barrierCount);
			}
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int nextIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex != nextIndex, void> addBeginBarriersOneResourceHelper2(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			if (std::get<previousIndex>(subPasses).isInView(sharedResources))
			{
				addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY>(sharedResources, std::get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
			}
			else
			{
				constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
				addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, nextIndex, (previousIndex + 1u) % subPassCount>(sharedResources,
					subPasses, barriers, barrierCount);
			}
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int nextIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex == nextIndex, void> addBeginBarriersOneResourceHelper2(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAG_NONE>(sharedResources, std::get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex == dependencyCount, void> addBeginBarriersOneResourceHelper(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousSubPassIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::state;
			if (stateBefore != stateAfter)
			{
				constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
				addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, startIndex, (previousSubPassIndex + 1u) % subPassCount>(sharedResources,
					subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex>
		static void addBeginBarriers(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? (subPassCount - 1u) : nextIndex - 1u;
			addBeginBarriersHelper1<previousIndex, nextIndex>(sharedResources, subPasses, barriers, barrierCount);
		}

		template<unsigned int previousIndex, unsigned int nextIndex>
		static void addBeginBarriersHelper1(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			if (std::get<previousIndex>(subPasses).isInView(sharedResources))
			{
				using SubPass = std::tuple_element_t<previousIndex, std::tuple<RenderSubPass_t...>>;
				using Dependencies = decltype(std::tuple_cat(std::declval<typename SubPass::Dependencies>(),
					std::declval<std::tuple<std::integral_constant<unsigned int, previousIndex>>>()));
				addBeginBarriersHelper2<previousIndex, 0u, std::tuple_size<Dependencies>::value, Dependencies>(sharedResources, subPasses, barriers, barrierCount);
			}
			else
			{
				addBeginBarriersHelper1<previousIndex == 0u ? (subPassCount - 1u) : previousIndex - 1u, nextIndex>(sharedResources, subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int previousIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex != dependencyIndexEnd, void> addBeginBarriersHelper2(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependency = std::tuple_element_t<dependencyIndex, Dependencies>::value;
			addBeginBarriersOneResource<previousIndex, (previousIndex + 1u) % subPassCount, dependency>(sharedResources, subPasses, barriers, barrierCount);
			addBeginBarriersHelper2<previousIndex, dependencyIndex + 1u, dependencyIndexEnd, Dependencies>(sharedResources, subPasses, barriers, barrierCount);
		}

		template<unsigned int nextSubPassIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex == dependencyIndexEnd, void> addBeginBarriersHelper2(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}


		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, D3D12_RESOURCE_BARRIER_FLAGS flags, class CameraSubPass>
		static void addBarrier(SharedResources& sharedResources, CameraSubPass& cameraSubPass, D3D12_RESOURCE_BARRIER* barriers,
			unsigned int& barrierCount)
		{
			auto cameras = cameraSubPass.cameras();
			auto camerasEnd = cameras.end();
			for (auto cam = cameras.begin(); cam != camerasEnd; ++cam)
			{
				auto camera = *cam;
				if (camera->isInView(sharedResources))
				{
					barriers[barrierCount].Flags = flags;
					barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barriers[barrierCount].Transition.pResource = camera->getImage();
					barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[barrierCount].Transition.StateBefore = stateBefore;
					barriers[barrierCount].Transition.StateAfter = stateAfter;
					++barrierCount;
				}
			}
		}

		template<unsigned int nextIndex, unsigned int currentIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentIndex == endIndex, void> addEndOnlyBarriers(SharedResources& sharedResources,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}

		template<unsigned int previousIndex, unsigned int nextIndex, unsigned int transitioningResourceIndex>
		static void addEndOnlyBarriersOneResource(SharedResources& sharedResources,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			using PreviousSubPass = std::tuple_element_t<previousIndex, SubPasses>;
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependecyCount = std::tuple_size<typename PreviousSubPass::Dependencies>::value;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>();
			if (std::get<previousIndex>(subPasses).isInView(sharedResources) && (dependencyIndex != dependecyCount || previousIndex == transitioningResourceIndex)) //needs to check that next index after previousIndex Not count empty subPasses != nextIndex
			{
				using SubPasses = std::tuple<RenderSubPass_t...>;
				using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
				using PreviousSubPass = std::tuple_element_t<previousIndex, SubPasses>;
				constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousIndex>();
				constexpr D3D12_RESOURCE_STATES stateAfter = getStateAfterEndBarriars<transitioningResourceIndex, NextSubPass >();
				if (stateBefore != stateAfter)
				{
					addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY>(sharedResources, std::get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
				}
			}
			else if (previousIndex != nextIndex)
			{
				addEndOnlyBarriersOneResource<previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u, nextIndex, transitioningResourceIndex>(sharedResources,
					subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex>
		static void addEndOnlyBarriers(SharedResources& sharedResources, std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers,
			unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			using Dependencies = decltype(std::tuple_cat(std::declval<typename NextSubPass::Dependencies>(),
				std::declval<std::tuple<std::integral_constant<unsigned int, nextIndex>>>()));

			addEndOnlyBarriers<nextIndex, 0u, std::tuple_size<Dependencies>::value, Dependencies>(sharedResources, subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex, unsigned int currentResourceIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentResourceIndex != endIndex, void> addEndOnlyBarriers(SharedResources& sharedResources,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? subPassCount - 1u : nextIndex - 1u;
			constexpr unsigned int transitioningResourceIndex = std::tuple_element_t<currentResourceIndex, ResourceIndices>::value;
			addEndOnlyBarriersHelper<nextIndex, transitioningResourceIndex, previousIndex>(sharedResources, subPasses, barriers, barrierCount);
			addEndOnlyBarriers<nextIndex, currentResourceIndex + 1u, endIndex, ResourceIndices>(sharedResources, subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex != nextIndex, void> addEndOnlyBarriersHelper(SharedResources& sharedResources,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previouspreviousIndex = previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u;
			if (std::get<previousIndex>(subPasses).isInView(sharedResources))
			{
				addEndOnlyBarriersOneResource<previouspreviousIndex, nextIndex, transitioningResourceIndex>(sharedResources, subPasses, barriers, barrierCount);
			}
			else
			{
				addEndOnlyBarriersHelper<nextIndex, transitioningResourceIndex, previouspreviousIndex>(sharedResources, subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex == nextIndex, void> addEndOnlyBarriersHelper(SharedResources& sharedResources,
			std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount) {}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex>
		constexpr static std::enable_if_t<transitioningResourceIndex == previousSubPassIndex, D3D12_RESOURCE_STATES> getStateBeforeBarriers() noexcept
		{
			return std::tuple_element_t<previousSubPassIndex, std::tuple<RenderSubPass_t...>>::stateAfter;
		}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex>
		constexpr static std::enable_if_t<transitioningResourceIndex != previousSubPassIndex, D3D12_RESOURCE_STATES> getStateBeforeBarriers() noexcept
		{
			using PreviousSubPass = std::tuple_element_t<previousSubPassIndex, std::tuple<RenderSubPass_t...>>;
			return getStateBeforeBarriersHelper<transitioningResourceIndex, previousSubPassIndex, findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>(),
				std::tuple_size<typename PreviousSubPass::Dependencies>::value>();
		}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex, unsigned int dependencyIndex, unsigned int dependecyCount>
		constexpr static std::enable_if_t<dependencyIndex != dependecyCount, D3D12_RESOURCE_STATES> getStateBeforeBarriersHelper() noexcept
		{
			using PreviousSubPass = std::tuple_element_t<previousSubPassIndex, std::tuple<RenderSubPass_t...>>;
			return std::tuple_element_t<findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>(), typename PreviousSubPass::DependencyStates>::value;
		}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex, unsigned int dependencyIndex, unsigned int dependecyCount>
		constexpr static std::enable_if_t<dependencyIndex == dependecyCount, D3D12_RESOURCE_STATES> getStateBeforeBarriersHelper() noexcept
		{
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		}

		/*
			Adds all resource barriers for one subPass
		*/
		template<unsigned int nextIndex>
		static void addBarriers(SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, unsigned int& barrierCount)
		{
			addBeginBarriers<nextIndex>(sharedResources, renderPass.subPasses, renderPass.barriers.get(), barrierCount);
			addEndOnlyBarriers<nextIndex>(sharedResources, renderPass.subPasses, renderPass.barriers.get(), barrierCount);
		}
	public:
		ThreadLocal(SharedResources& sharedResources) : subPassesThreadLocal((sizeof(typename RenderSubPass_t::ThreadLocal), sharedResources)...) {}

		void update1(BaseExecutor* const executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int notFirstThread)
		{
			if (notFirstThread == 0u)
			{
				renderPass.firstExector = executor;
				renderPass.commandListCount = 0u;
			}
		}

		void update1After(SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature, unsigned int notFirstThread)
		{
			if (notFirstThread == 0u)
			{
				update1After<0u, sizeof...(RenderSubPass_t), true>(sharedResources, renderPass, rootSignature);
			}
			else
			{
				update1After<0u, sizeof...(RenderSubPass_t), false>(sharedResources, renderPass, rootSignature);
			}
		}

		template<class Executor, class SharedResources>
		void update2(Executor* const executor, SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			const unsigned int threadCount = (1u + sharedResources.maxPrimaryThreads + sharedResources.numPrimaryJobExeThreads);
			if (executor == renderPass.firstExector)
			{
				if (threadCount == 1u)
				{
					update2LastThread<0u, subPassCount>(executor, sharedResources, renderPass, renderPass.commandLists.get(), threadCount);
				}
				else
				{
					update2<0u, subPassCount>(sharedResources, renderPass, renderPass.commandLists.get(), threadCount);
				}
			}
			else
			{
				++renderPass.commandListCount;
				if (threadCount == 1u + renderPass.commandListCount)
				{
					update2LastThread<0u, subPassCount>(executor, sharedResources, renderPass, renderPass.commandLists.get() + renderPass.commandListCount, threadCount);
				}
				else
				{
					update2<0u, subPassCount>(sharedResources, renderPass, renderPass.commandLists.get() + renderPass.commandListCount, threadCount);
				}
			}
		}

		template<class Executor, class SharedResources>
		void update2LastThread(Executor* const executor, SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass)
		{
			update2(executor, sharedResources, renderPass);
			unsigned int commandListCount = commandListPerThread(renderPass.subPasses, sharedResources) *
				(1u + sharedResources.maxPrimaryThreads + sharedResources.numPrimaryJobExeThreads);
			sharedResources.graphicsEngine.present(sharedResources.window, renderPass.commandLists.get(), commandListCount);
		}
	private:
		unsigned int commandListPerThread(std::tuple<RenderSubPass_t...>& subPasses, SharedResources& sharedResources)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			return commandListPerThread<0u, subPassCount>(subPasses, sharedResources);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start != end, unsigned int> commandListPerThread(std::tuple<RenderSubPass_t...>& subPasses, SharedResources& sharedResources)
		{
			auto& subPass = std::get<start>(subPasses);
			if (subPass.isInView(sharedResources))
			{
				return subPass.commandListsPerFrame + commandListPerThread<start + 1u, end>(subPasses, sharedResources);
			}
			return commandListPerThread<start + 1u, end>(subPasses, sharedResources);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, unsigned int> commandListPerThread(std::tuple<RenderSubPass_t...>& subPasses, SharedResources& sharedResources)
		{
			return 0u;
		}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<!(start != end), void> update1After(SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature) {}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<start != end, void>
			update1After(SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature)
		{
			auto& subPassLocal = std::get<start>(subPassesThreadLocal);
			auto& subPass = std::get<start>(renderPass.subPasses);
			if (subPass.isInView(sharedResources))
			{
				if (firstThread)
				{
					unsigned int barriarCount = 0u;
					addBarriers<start>(sharedResources, renderPass, barriarCount);
					subPassLocal.update1AfterFirstThread(sharedResources, subPass, rootSignature, barriarCount, renderPass.barriers.get());
				}
				else
				{
					subPassLocal.update1After(sharedResources, subPass, rootSignature);
				}
			}
			update1After<start + 1u, end, firstThread>(sharedResources, renderPass, rootSignature);
		}
	
		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, void>
			update2(SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int numThreads) {}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start != end, void>
			update2(SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int numThreads)
		{
			const auto commandListsPerFrame = std::get<start>(renderPass.subPasses).commandListsPerFrame;
			auto& subPassLocal = std::get<start>(subPassesThreadLocal);
			auto& subPass = std::get<start>(renderPass.subPasses);
			if (subPass.isInView(sharedResources)) //currently first thread first list, second thread first list... end first thread second list etc. but should be all first list then all second lists etc.
			{
				subPassLocal.update2(commandLists, numThreads);
			}
			update2<start + 1u, end>(sharedResources, renderPass, commandLists, numThreads);
		}


		template<unsigned int start, unsigned int end, class Executor, class SharedResources>
		std::enable_if_t<start == end, void>
			update2LastThread(Executor* const executor, SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int listsBefore) {}

		template<unsigned int start, unsigned int end, class Executor, class SharedResources>
		std::enable_if_t<start != end, void>
			update2LastThread(Executor* const executor, SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int numThreads)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			auto& subPass = std::get<start>(renderPass.subPasses);
			if (subPass.isInView(sharedResources))
			{
				auto& subPassLocal = std::get<start>(subPassesThreadLocal);
				subPassLocal.update2LastThread(commandLists, numThreads, subPass, executor, sharedResources);
			}
			update2LastThread<start + 1u, end>(executor, sharedResources, renderPass, commandLists, numThreads);
		}
	};

	void update1(D3D12GraphicsEngine& graphicsEngine)
	{
		graphicsEngine.waitForPreviousFrame();
	}
};