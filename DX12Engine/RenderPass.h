#pragma once
#include <d3d12.h>
#include <memory>
#include "for_each.h"
#include <utility>
#include <atomic>
#include "GraphicsEngine.h"
#include "ActorQueue.h"
#include "RenderPassMessage.h"
#include "PrimaryTaskFromOtherThreadQueue.h"
class Window;

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4127)
#endif

template<class... RenderSubPass_t>
class RenderPass : private PrimaryTaskFromOtherThreadQueue::Task
{
	std::unique_ptr<ID3D12CommandList*[]> commandLists;//size = subPassCount * threadCount * commandListsPerFrame
	std::atomic<unsigned int> mCurrentIndex = 0u;
	void* firstExector;
	unsigned int maxBarriers = 0u;
	std::unique_ptr<D3D12_RESOURCE_BARRIER[]> barriers;//size = maxDependences * 2u * maxCameras * maxCommandListsPerFrame
	std::tuple<RenderSubPass_t...> mSubPasses;
	ActorQueue messageQueue;

	template<class ThreadResources, class GlobalResources>
	void update1(ThreadResources& threadResources, GlobalResources& globalResources)
	{
		globalResources.graphicsEngine.startFrame(&threadResources, &globalResources);
	}

	void run(void* tr, void* gr)
	{
		do
		{
			SinglyLinked* temp = messageQueue.popAll();
			while(temp != nullptr)
			{
				auto& message = *static_cast<RenderPassMessage*>(temp);
				temp = temp->next; //Allow reuse of next
				message.execute(message, tr, gr);
				updateBarrierCount();
			}
		} while(!messageQueue.stop());
	}

	void updateBarrierCount()
	{
		//grow barriers if needed
		unsigned int maxDependences = 0u, maxCameras = 0u, maxCommandListsPerFrame = 0u;
		for_each(mSubPasses, [&maxDependences, &maxCameras, &maxCommandListsPerFrame](auto& subPass)
		{
			if(std::tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value > maxDependences)
			{
				maxDependences = (unsigned int)std::tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value;
			}
			if(subPass.cameraCount() > maxCameras)
			{
				maxCameras = subPass.cameraCount();
			}
			if(subPass.commandListsPerFrame > maxCommandListsPerFrame)
			{
				maxCommandListsPerFrame = subPass.commandListsPerFrame;
			}
		});

		auto newSize = maxDependences * maxCameras * maxCommandListsPerFrame * 2u;
		if(newSize > maxBarriers)
		{
			barriers.reset(new D3D12_RESOURCE_BARRIER[newSize]);
			maxBarriers = newSize;
		}
	}
public:
	RenderPass() {}
	RenderPass(unsigned int threadCount)
	{
		unsigned int commandListCount = 0u;
		for_each(mSubPasses, [&commandListCount](auto& subPass)
		{
			commandListCount += subPass.commandListsPerFrame;
		});
		commandListCount *= threadCount;
		commandLists.reset(new ID3D12CommandList*[commandListCount]);
		updateBarrierCount();
	}

	template<class ThreadResources, class GlobalResources>
	void addMessage(RenderPassMessage& message, ThreadResources& threadResources, GlobalResources&)
	{
		bool needsStarting = messageQueue.push(&message);
		if(needsStarting)
		{
			threadResources.taskShedular.pushPrimaryTask(0u, {this, [](void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& pass = *static_cast<RenderPass*>(requester);
				pass.run(&threadResources, &globalResources);
			}});
		}
	}

	template<class ThreadResources, class GlobalResources>
	void addMessageFromBackground(RenderPassMessage& message, ThreadResources&, GlobalResources& globalResources)
	{
		bool needsStarting = messageQueue.push(&message);
		if(needsStarting)
		{
			execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr, void* gr)
			{
				auto& pass = static_cast<RenderPass&>(task);
				pass.run(tr, gr);
			};
			globalResources.taskShedular.pushPrimaryTaskFromOtherThread(0u, *this);
		}
	}

	std::tuple<RenderSubPass_t...>& subPasses()
	{
		return mSubPasses;
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
		static void addBeginBarriersOneResource(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPass = std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>;
			using Dependencies = typename SubPass::Dependencies;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, Dependencies>();
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependencyCount = std::tuple_size<Dependencies>::value;
			if (std::get<startIndex>(subPasses).isInView() && (dependencyIndex != dependencyCount || transitioningResourceIndex == startIndex))
			{
				addBeginBarriersOneResourceHelper<previousSubPassIndex, startIndex, transitioningResourceIndex, dependencyIndex, dependencyCount>(subPasses,
					barriers, barrierCount);
			}
			else if (startIndex != previousSubPassIndex)
			{
				addBeginBarriersOneResource<previousSubPassIndex, startIndex == subPassCount - 1u ? 0u : startIndex + 1u, transitioningResourceIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex != dependencyCount, void> addBeginBarriersOneResourceHelper(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousSubPassIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = std::tuple_element_t<dependencyIndex, typename std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::DependencyStates>::value;
			if (stateBefore != stateAfter)
			{
				constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
				addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, startIndex, (previousSubPassIndex + 1u) % subPassCount>(subPasses, barriers, barrierCount);
			}
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int nextIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex != nextIndex, void> addBeginBarriersOneResourceHelper2(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			if (std::get<previousIndex>(subPasses).isInView())
			{
				addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY>(std::get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
			}
			else
			{
				constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
				addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, nextIndex, (previousIndex + 1u) % subPassCount>(subPasses, barriers, barrierCount);
			}
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int nextIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex == nextIndex, void> addBeginBarriersOneResourceHelper2(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAG_NONE>(std::get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex == dependencyCount, void> addBeginBarriersOneResourceHelper(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousSubPassIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = std::tuple_element_t<startIndex, std::tuple<RenderSubPass_t...>>::state;
			if (stateBefore != stateAfter)
			{
				constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
				addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, startIndex, (previousSubPassIndex + 1u) % subPassCount>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex>
		static void addBeginBarriers(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? (subPassCount - 1u) : nextIndex - 1u;
			addBeginBarriersHelper1<previousIndex, nextIndex>(subPasses, barriers, barrierCount);
		}

		template<unsigned int previousIndex, unsigned int nextIndex>
		static void addBeginBarriersHelper1(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			if (std::get<previousIndex>(subPasses).isInView())
			{
				using SubPass = std::tuple_element_t<previousIndex, std::tuple<RenderSubPass_t...>>;
				using Dependencies = decltype(std::tuple_cat(std::declval<typename SubPass::Dependencies>(),
					std::declval<std::tuple<std::integral_constant<unsigned int, previousIndex>>>()));
				addBeginBarriersHelper2<previousIndex, 0u, std::tuple_size<Dependencies>::value, Dependencies>(subPasses, barriers, barrierCount);
			}
			else
			{
				addBeginBarriersHelper1<previousIndex == 0u ? (subPassCount - 1u) : previousIndex - 1u, nextIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int previousIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex != dependencyIndexEnd, void> addBeginBarriersHelper2(std::tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependency = std::tuple_element_t<dependencyIndex, Dependencies>::value;
			addBeginBarriersOneResource<previousIndex, (previousIndex + 1u) % subPassCount, dependency>(subPasses, barriers, barrierCount);
			addBeginBarriersHelper2<previousIndex, dependencyIndex + 1u, dependencyIndexEnd, Dependencies>(subPasses, barriers, barrierCount);
		}

		template<unsigned int nextSubPassIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex == dependencyIndexEnd, void> addBeginBarriersHelper2(std::tuple<RenderSubPass_t...>&,
			D3D12_RESOURCE_BARRIER*, unsigned int&) {}


		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, D3D12_RESOURCE_BARRIER_FLAGS flags, class CameraSubPass>
		static void addBarrier(CameraSubPass& cameraSubPass, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			auto cameras = cameraSubPass.cameras();
			auto camerasEnd = cameras.end();
			for (auto cam = cameras.begin(); cam != camerasEnd; ++cam)
			{
				auto& camera = *cam;
				if (camera.isInView())
				{
					barriers[barrierCount].Flags = flags;
					barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barriers[barrierCount].Transition.pResource = camera.getImage();
					barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[barrierCount].Transition.StateBefore = stateBefore;
					barriers[barrierCount].Transition.StateAfter = stateAfter;
					++barrierCount;
				}
			}
		}

		template<unsigned int nextIndex, unsigned int currentIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentIndex == endIndex, void> addEndOnlyBarriers(std::tuple<RenderSubPass_t...>&, D3D12_RESOURCE_BARRIER*, unsigned int&) {}

		template<unsigned int previousIndex, unsigned int nextIndex, unsigned int transitioningResourceIndex>
		static void addEndOnlyBarriersOneResource(std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			using PreviousSubPass = std::tuple_element_t<previousIndex, SubPasses>;
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependecyCount = std::tuple_size<typename PreviousSubPass::Dependencies>::value;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>();
			if (std::get<previousIndex>(subPasses).isInView() && (dependencyIndex != dependecyCount || previousIndex == transitioningResourceIndex)) //needs to check that next index after previousIndex Not count empty subPasses != nextIndex
			{
				using SubPasses = std::tuple<RenderSubPass_t...>;
				using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
				using PreviousSubPass = std::tuple_element_t<previousIndex, SubPasses>;
				constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousIndex>();
				constexpr D3D12_RESOURCE_STATES stateAfter = getStateAfterEndBarriars<transitioningResourceIndex, NextSubPass >();
				if (stateBefore != stateAfter)
				{
					addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY>(std::get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
				}
			}
			else if (previousIndex != nextIndex)
			{
				addEndOnlyBarriersOneResource<previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u, nextIndex, transitioningResourceIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex>
		static void addEndOnlyBarriers(std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers,
			unsigned int& barrierCount)
		{
			using SubPasses = std::tuple<RenderSubPass_t...>;
			using NextSubPass = std::tuple_element_t<nextIndex, SubPasses>;
			using Dependencies = decltype(std::tuple_cat(std::declval<typename NextSubPass::Dependencies>(),
				std::declval<std::tuple<std::integral_constant<unsigned int, nextIndex>>>()));

			addEndOnlyBarriers<nextIndex, 0u, std::tuple_size<Dependencies>::value, Dependencies>(subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex, unsigned int currentResourceIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentResourceIndex != endIndex, void> addEndOnlyBarriers(std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? subPassCount - 1u : nextIndex - 1u;
			constexpr unsigned int transitioningResourceIndex = std::tuple_element_t<currentResourceIndex, ResourceIndices>::value;
			addEndOnlyBarriersHelper<nextIndex, transitioningResourceIndex, previousIndex>(subPasses, barriers, barrierCount);
			addEndOnlyBarriers<nextIndex, currentResourceIndex + 1u, endIndex, ResourceIndices>(subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex != nextIndex, void> addEndOnlyBarriersHelper(std::tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previouspreviousIndex = previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u;
			if (std::get<previousIndex>(subPasses).isInView())
			{
				addEndOnlyBarriersOneResource<previouspreviousIndex, nextIndex, transitioningResourceIndex>(subPasses, barriers, barrierCount);
			}
			else
			{
				addEndOnlyBarriersHelper<nextIndex, transitioningResourceIndex, previouspreviousIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex == nextIndex, void> addEndOnlyBarriersHelper(std::tuple<RenderSubPass_t...>&, D3D12_RESOURCE_BARRIER*, unsigned int&) {}

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
		static void addBarriers(RenderPass<RenderSubPass_t...>& renderPass, unsigned int& barrierCount)
		{
			addBeginBarriers<nextIndex>(renderPass.mSubPasses, renderPass.barriers.get(), barrierCount);
			addEndOnlyBarriers<nextIndex>(renderPass.mSubPasses, renderPass.barriers.get(), barrierCount);
		}
	public:
		ThreadLocal(GraphicsEngine& graphicsEngine) : subPassesThreadLocal((sizeof(typename RenderSubPass_t::ThreadLocal), graphicsEngine)...) {}

		template<class ThreadResources, class GlobalResources>
		void update1(ThreadResources& threadResources, GlobalResources& globalResources, RenderPass<RenderSubPass_t...>& renderPass, bool firstThread)
		{
			if (firstThread)
			{
				renderPass.firstExector = &threadResources;
				renderPass.mCurrentIndex.store(0u, std::memory_order::memory_order_relaxed);
				renderPass.update1(threadResources, globalResources);
			}
		}

		void update1After(GraphicsEngine& graphicsEngine, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature, bool firstThread)
		{
			if (firstThread)
			{
				update1After<0u, sizeof...(RenderSubPass_t), true>(graphicsEngine, renderPass, rootSignature);
			}
			else
			{
				update1After<0u, sizeof...(RenderSubPass_t), false>(graphicsEngine, renderPass, rootSignature);
			}
		}

		template<class Executor, class SharedResources>
		void update2(Executor& executor, SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, unsigned int threadCount)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			if (&executor == (Executor*)renderPass.firstExector)
			{
				if (threadCount == 1u)
				{
					update2LastThread<0u, subPassCount>(executor, sharedResources, renderPass, renderPass.commandLists.get(), threadCount);
				}
				else
				{
					update2<0u, subPassCount>(renderPass, renderPass.commandLists.get(), threadCount);
				}
			}
			else
			{
				unsigned int index = renderPass.mCurrentIndex.fetch_add(1u, std::memory_order_relaxed) + 1u;
				if (threadCount == 1u + index)
				{
					update2LastThread<0u, subPassCount>(executor, sharedResources, renderPass, renderPass.commandLists.get() + index, threadCount);
				}
				else
				{
					update2<0u, subPassCount>(renderPass, renderPass.commandLists.get() + index, threadCount);
				}
			}
		}

		void present(unsigned int primaryThreadCount, GraphicsEngine& graphicsEngine, Window& window, RenderPass<RenderSubPass_t...>& renderPass)
		{
			unsigned int commandListCount = commandListPerThread(renderPass.mSubPasses) * primaryThreadCount;
			graphicsEngine.endFrame(window, renderPass.commandLists.get(), commandListCount);
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
			auto& subPass = std::get<start>(subPasses);
			if (subPass.isInView())
			{
				return subPass.commandListsPerFrame + commandListPerThread<start + 1u, end>(subPasses);
			}
			return commandListPerThread<start + 1u, end>(subPasses);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, unsigned int> commandListPerThread(std::tuple<RenderSubPass_t...>&)
		{
			return 0u;
		}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<!(start != end), void> update1After(GraphicsEngine&, RenderPass<RenderSubPass_t...>&, ID3D12RootSignature*) {}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<start != end, void>
			update1After(GraphicsEngine& graphicsEngine, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature)
		{
			auto& subPassLocal = std::get<start>(subPassesThreadLocal);
			auto& subPass = std::get<start>(renderPass.mSubPasses);
			if (subPass.isInView())
			{
				if (firstThread)
				{
					unsigned int barriarCount = 0u;
					addBarriers<start>(renderPass, barriarCount);
					subPassLocal.update1AfterFirstThread(graphicsEngine, subPass, rootSignature, barriarCount, renderPass.barriers.get());
				}
				else
				{
					subPassLocal.update1After(graphicsEngine, subPass, rootSignature);
				}
			}
			update1After<start + 1u, end, firstThread>(graphicsEngine, renderPass, rootSignature);
		}
	
		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, void>
			update2(RenderPass<RenderSubPass_t...>&, ID3D12CommandList**,
				unsigned int) {}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start != end, void>
			update2(RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int numThreads)
		{
			const auto commandListsPerFrame = std::get<start>(renderPass.mSubPasses).commandListsPerFrame;
			auto& subPassLocal = std::get<start>(subPassesThreadLocal);
			auto& subPass = std::get<start>(renderPass.mSubPasses);
			if (subPass.isInView())
			{
				subPassLocal.update2(commandLists, numThreads);
			}
			update2<start + 1u, end>(renderPass, commandLists, numThreads);
		}


		template<unsigned int start, unsigned int end, class Executor, class SharedResources>
		std::enable_if_t<start == end, void>
			update2LastThread(Executor&, SharedResources&, RenderPass<RenderSubPass_t...>&, ID3D12CommandList**,
				unsigned int) {}

		template<unsigned int start, unsigned int end, class Executor, class SharedResources>
		std::enable_if_t<start != end, void>
			update2LastThread(Executor& executor, SharedResources& sharedResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int numThreads)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			auto& subPass = std::get<start>(renderPass.mSubPasses);
			if (subPass.isInView())
			{
				auto& subPassLocal = std::get<start>(subPassesThreadLocal);
				subPassLocal.update2LastThread(commandLists, numThreads, subPass, executor, sharedResources);
			}
			update2LastThread<start + 1u, end>(executor, sharedResources, renderPass, commandLists, numThreads);
		}
	};
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif