#pragma once
#include <d3d12.h>
#include <memory>
#include "for_each.h"
#include "Tuple.h"
#include <utility>
#include <type_traits>
#include <atomic>
#include "GraphicsEngine.h"
#include "ActorQueue.h"
#include "LinkedTask.h"
#include "PrimaryTaskFromOtherThreadQueue.h"
#include "TaskShedular.h"
class Window;

template<class... RenderSubPass_t>
class RenderPass : private PrimaryTaskFromOtherThreadQueue::Task
{
	std::unique_ptr<ID3D12CommandList*[]> commandLists;//size = subPassCount * threadCount * commandListsPerFrame
	unsigned int maxBarriers = 0u;
	std::unique_ptr<D3D12_RESOURCE_BARRIER[]> barriers;//size = maxDependences * 2u * maxCameras * maxCommandListsPerFrame
	Tuple<RenderSubPass_t...> mSubPasses;
	ActorQueue messageQueue;

	void run(void* tr)
	{
		do
		{
			SinglyLinked* temp = messageQueue.popAll();
			while(temp != nullptr)
			{
				auto& message = *static_cast<LinkedTask*>(temp);
				temp = temp->next; //Allow reuse of next
				message.execute(message, tr);
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
			if(tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value > maxDependences)
			{
				maxDependences = (unsigned int)tuple_size<typename std::remove_reference_t<decltype(subPass)>::Dependencies>::value;
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

	template<class SubPass, class ArgsTuple, std::size_t... indices>
	SubPass makeSubPass(std::index_sequence<indices...>, ArgsTuple&& parameters)
	{
		return { std::forward<tuple_element_t<indices, ArgsTuple>>(get<indices>(parameters))... };
	}

	template<class SubPass, class ArgsTuple>
	SubPass makeSubPass(ArgsTuple&& parameters)
	{
		return makeSubPass<SubPass>(std::make_index_sequence<tuple_size<std::remove_reference_t<ArgsTuple>>::value>(), std::forward<ArgsTuple>(parameters));
	}

	template<class ArgsTuple, std::size_t... indices>
	RenderPass(unsigned int threadCount, std::index_sequence<indices...>, ArgsTuple&& parameterPacks) :
		mSubPasses{ makeSubPass<tuple_element_t<indices, Tuple<RenderSubPass_t...>>,
			tuple_element_t<indices, ArgsTuple>>(std::forward<tuple_element_t<indices, ArgsTuple>>(get<indices>(parameterPacks)))... }
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
public:
	/*
	Each parameter after threadCount should be a tuple containing the arguments required to construct a subPass
	*/
	template<class... ParameterPacks>
	RenderPass(unsigned int threadCount, ParameterPacks&&... parameterPacks) :
		RenderPass(threadCount, std::make_index_sequence<sizeof...(ParameterPacks)>(),
			forward_as_tuple(std::forward<ParameterPacks>(parameterPacks)...))
	{}

	template<class ThreadResources>
	void addMessage(LinkedTask& message, ThreadResources& threadResources)
	{
		bool needsStarting = messageQueue.push(&message);
		if(needsStarting)
		{
			threadResources.taskShedular.pushPrimaryTask(0u, {this, [](void* requester, ThreadResources& threadResources)
			{
				auto& pass = *static_cast<RenderPass*>(requester);
				pass.run(&threadResources);
			}});
		}
	}

	template<class ThreadResources>
	void addMessageFromBackground(LinkedTask& message, TaskShedular<ThreadResources>& taskShedular)
	{
		bool needsStarting = messageQueue.push(&message);
		if(needsStarting)
		{
			execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr)
			{
				auto& pass = static_cast<RenderPass&>(task);
				pass.run(tr);
			};
			taskShedular.pushPrimaryTaskFromOtherThread(0u, *this);
		}
	}

	Tuple<RenderSubPass_t...>& subPasses()
	{
		return mSubPasses;
	}
private:
	template<class ArgsTuple, class SubPass, std::size_t... indices>
	static void resizeSubPass(ArgsTuple&& args, SubPass& subPass, std::index_sequence<indices...>)
	{
		subPass.resize(get<indices>(args)...);
	}

	template<unsigned int i, unsigned int end, class ArgsTuplesTuple>
	void resize([[maybe_unused]] ArgsTuplesTuple&& argsTuplesTuple)
	{
		if constexpr (i != end)
		{
			resizeSubPass(get<i>(std::forward<ArgsTuplesTuple>(argsTuplesTuple)), get<i>(mSubPasses), std::make_index_sequence<tuple_size_v<std::remove_reference_t<decltype(get<i>(argsTuplesTuple))>>>());
			resize<i + 1u, end, ArgsTuplesTuple>(std::forward<ArgsTuplesTuple>(argsTuplesTuple));
		}
	}
public:
	template<class... ArgsTuples>
	void resize(ArgsTuples&&... argsTuples)
	{
		resize<0, sizeof...(RenderSubPass_t), Tuple<ArgsTuples&&...>>(forward_as_tuple(std::forward<ArgsTuples>(argsTuples)...));
	}

	class ThreadLocal
	{
	public:
		Tuple<typename RenderSubPass_t::ThreadLocal...> subPassesThreadLocal;
	private:
		template <class, class>
		struct JoinTupleElements;
		template <template<class...> class T1, template<class...> class T2, class... First, class... Second>
		struct JoinTupleElements<T1<First...>, T2<Second...>> {
			using type = Tuple<First..., Second...>;
		};

		template<unsigned int index, class Dependencies, unsigned int start = 0u, unsigned int end = tuple_size<Dependencies>::value>
		constexpr static std::enable_if_t<start != end, unsigned int> findDependency()
		{
			return tuple_element_t<start, Dependencies>::value == index ? start : findDependency<index, Dependencies, start + 1u, end>();
		}

		template<unsigned int index, class Dependencies, unsigned int start = 0u, unsigned int end = tuple_size<Dependencies>::value>
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
		constexpr static D3D12_RESOURCE_STATES getStateAfterEndBarriarsHelper()
		{
			if constexpr (stateAfterIndex == tuple_size<typename NextSubPass::Dependencies>::value)
			{
				return NextSubPass::state;
			}
			else
			{
				return tuple_element_t<stateAfterIndex, typename NextSubPass::DependencyStates>::value;
			}
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex>
		static void addBeginBarriersOneResource(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPass = tuple_element_t<startIndex, Tuple<RenderSubPass_t...>>;
			using Dependencies = typename SubPass::Dependencies;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, Dependencies>();
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependencyCount = tuple_size<Dependencies>::value;
			if (get<startIndex>(subPasses).isInView() && (dependencyIndex != dependencyCount || transitioningResourceIndex == startIndex))
			{
				addBeginBarriersOneResourceHelper<previousSubPassIndex, startIndex, transitioningResourceIndex, dependencyIndex, dependencyCount>(subPasses,
					barriers, barrierCount);
			}
			else if constexpr (startIndex != previousSubPassIndex)
			{
				addBeginBarriersOneResource<previousSubPassIndex, startIndex == subPassCount - 1u ? 0u : startIndex + 1u, transitioningResourceIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int nextIndex, unsigned int previousIndex>
		static std::enable_if_t<stateBefore != stateAfter && previousIndex != nextIndex, void> addBeginBarriersOneResourceHelper2(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			if (get<previousIndex>(subPasses).isInView())
			{
				addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY>(get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
			}
			else
			{
				constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
				addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, nextIndex, (previousIndex + 1u) % subPassCount>(subPasses, barriers, barrierCount);
			}
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int nextIndex, unsigned int previousIndex>
		static std::enable_if_t<stateBefore != stateAfter && previousIndex == nextIndex, void> addBeginBarriersOneResourceHelper2(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAG_NONE>(get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
		}

		template<D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, unsigned int transitioningResourceIndex, unsigned int nextIndex, unsigned int previousIndex>
		static std::enable_if_t<stateBefore == stateAfter, void> addBeginBarriersOneResourceHelper2(Tuple<RenderSubPass_t...>&, D3D12_RESOURCE_BARRIER*, unsigned int&) {}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex != dependencyCount, void> addBeginBarriersOneResourceHelper(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousSubPassIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = tuple_element_t<dependencyIndex, typename tuple_element_t<startIndex, Tuple<RenderSubPass_t...>>::DependencyStates>::value;
			
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, startIndex, (previousSubPassIndex + 1u) % subPassCount>(subPasses, barriers, barrierCount);
		}

		template<unsigned int previousSubPassIndex, unsigned int startIndex, unsigned int transitioningResourceIndex, unsigned int dependencyIndex, unsigned int dependencyCount>
		static std::enable_if_t<dependencyIndex == dependencyCount, void> addBeginBarriersOneResourceHelper(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousSubPassIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = tuple_element_t<startIndex, Tuple<RenderSubPass_t...>>::state;
			
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			addBeginBarriersOneResourceHelper2<stateBefore, stateAfter, transitioningResourceIndex, startIndex, (previousSubPassIndex + 1u) % subPassCount>(subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex>
		static void addBeginBarriers(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? (subPassCount - 1u) : nextIndex - 1u;
			addBeginBarriersHelper1<previousIndex, nextIndex>(subPasses, barriers, barrierCount);
		}

		template<unsigned int previousIndex, unsigned int nextIndex>
		static void addBeginBarriersHelper1(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			if (get<previousIndex>(subPasses).isInView())
			{
				using SubPass = tuple_element_t<previousIndex, Tuple<RenderSubPass_t...>>;
				using Dependencies = typename JoinTupleElements<typename SubPass::Dependencies, Tuple<std::integral_constant<unsigned int, previousIndex>>>::type;
				addBeginBarriersHelper2<previousIndex, 0u, tuple_size<Dependencies>::value, Dependencies>(subPasses, barriers, barrierCount);
			}
			else
			{
				addBeginBarriersHelper1<previousIndex == 0u ? (subPassCount - 1u) : previousIndex - 1u, nextIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int previousIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex != dependencyIndexEnd, void> addBeginBarriersHelper2(Tuple<RenderSubPass_t...>& subPasses,
			D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependency = tuple_element_t<dependencyIndex, Dependencies>::value;
			addBeginBarriersOneResource<previousIndex, (previousIndex + 1u) % subPassCount, dependency>(subPasses, barriers, barrierCount);
			addBeginBarriersHelper2<previousIndex, dependencyIndex + 1u, dependencyIndexEnd, Dependencies>(subPasses, barriers, barrierCount);
		}

		template<unsigned int nextSubPassIndex, unsigned int dependencyIndex, unsigned int dependencyIndexEnd, class Dependencies>
		static std::enable_if_t<dependencyIndex == dependencyIndexEnd, void> addBeginBarriersHelper2(Tuple<RenderSubPass_t...>&,
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
					barriers[barrierCount].Transition.pResource = &camera.getImage();
					barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[barrierCount].Transition.StateBefore = stateBefore;
					barriers[barrierCount].Transition.StateAfter = stateAfter;
					++barrierCount;
				}
			}
		}

		template<unsigned int previousIndex, unsigned int nextIndex, unsigned int transitioningResourceIndex>
		static void addEndOnlyBarriersOneResource(Tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			using SubPasses = Tuple<RenderSubPass_t...>;
			using NextSubPass = tuple_element_t<nextIndex, SubPasses>;
			using PreviousSubPass = tuple_element_t<previousIndex, SubPasses>;
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr auto dependecyCount = tuple_size<typename PreviousSubPass::Dependencies>::value;
			constexpr auto dependencyIndex = findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>();
			constexpr D3D12_RESOURCE_STATES stateBefore = getStateBeforeBarriers<transitioningResourceIndex, previousIndex>();
			constexpr D3D12_RESOURCE_STATES stateAfter = getStateAfterEndBarriars<transitioningResourceIndex, NextSubPass>();
			if constexpr (dependencyIndex != dependecyCount || previousIndex == transitioningResourceIndex)
			{
				if (get<previousIndex>(subPasses).isInView())
				{
					if constexpr (stateBefore != stateAfter)
					{
						addBarrier<stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY>(get<transitioningResourceIndex>(subPasses), barriers, barrierCount);
					}
				}
				else
				{
					addEndOnlyBarriersOneResource<previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u, nextIndex, transitioningResourceIndex>(subPasses, barriers, barrierCount);
				}
			}
			else if constexpr (previousIndex != nextIndex)
			{
				addEndOnlyBarriersOneResource<previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u, nextIndex, transitioningResourceIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex, unsigned int currentIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentIndex == endIndex, void> addEndOnlyBarriers(Tuple<RenderSubPass_t...>&, D3D12_RESOURCE_BARRIER*, unsigned int&) {}

		template<unsigned int nextIndex, unsigned int currentIndex, unsigned int endIndex, class ResourceIndices>
		static std::enable_if_t<currentIndex != endIndex, void> addEndOnlyBarriers(Tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previousIndex = nextIndex == 0u ? subPassCount - 1u : nextIndex - 1u;
			constexpr unsigned int transitioningResourceIndex = tuple_element_t<currentIndex, ResourceIndices>::value;
			addEndOnlyBarriersHelper<nextIndex, transitioningResourceIndex, previousIndex>(subPasses, barriers, barrierCount);
			addEndOnlyBarriers<nextIndex, currentIndex + 1u, endIndex, ResourceIndices>(subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex>
		static void addEndOnlyBarriers(Tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers,
			unsigned int& barrierCount)
		{
			using SubPasses = Tuple<RenderSubPass_t...>;
			using NextSubPass = typename tuple_element<nextIndex, SubPasses>::type;
			using Dependencies = typename JoinTupleElements<typename NextSubPass::Dependencies, Tuple<std::integral_constant<unsigned int, nextIndex>>>::type;
			addEndOnlyBarriers<nextIndex, 0u, tuple_size<Dependencies>::value, Dependencies>(subPasses, barriers, barrierCount);
		}

		template<unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex != nextIndex, void> addEndOnlyBarriersHelper(Tuple<RenderSubPass_t...>& subPasses, D3D12_RESOURCE_BARRIER* barriers, unsigned int& barrierCount)
		{
			constexpr unsigned int subPassCount = sizeof...(RenderSubPass_t);
			constexpr unsigned int previouspreviousIndex = previousIndex == 0u ? subPassCount - 1u : previousIndex - 1u;
			if (get<previousIndex>(subPasses).isInView())
			{
				addEndOnlyBarriersOneResource<previouspreviousIndex, nextIndex, transitioningResourceIndex>(subPasses, barriers, barrierCount);
			}
			else
			{
				addEndOnlyBarriersHelper<nextIndex, transitioningResourceIndex, previouspreviousIndex>(subPasses, barriers, barrierCount);
			}
		}

		template<unsigned int nextIndex, unsigned int transitioningResourceIndex, unsigned int previousIndex>
		static std::enable_if_t<previousIndex == nextIndex, void> addEndOnlyBarriersHelper(Tuple<RenderSubPass_t...>&, D3D12_RESOURCE_BARRIER*, unsigned int&) {}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex>
		constexpr static std::enable_if_t<transitioningResourceIndex == previousSubPassIndex, D3D12_RESOURCE_STATES> getStateBeforeBarriers() noexcept
		{
			return tuple_element_t<previousSubPassIndex, Tuple<RenderSubPass_t...>>::stateAfter;
		}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex>
		constexpr static std::enable_if_t<transitioningResourceIndex != previousSubPassIndex, D3D12_RESOURCE_STATES> getStateBeforeBarriers() noexcept
		{
			using PreviousSubPass = tuple_element_t<previousSubPassIndex, Tuple<RenderSubPass_t...>>;
			return getStateBeforeBarriersHelper<transitioningResourceIndex, previousSubPassIndex, findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>(),
				tuple_size<typename PreviousSubPass::Dependencies>::value>();
		}

		template<unsigned int transitioningResourceIndex, unsigned int previousSubPassIndex, unsigned int dependencyIndex, unsigned int dependecyCount>
		constexpr static std::enable_if_t<dependencyIndex != dependecyCount, D3D12_RESOURCE_STATES> getStateBeforeBarriersHelper() noexcept
		{
			using PreviousSubPass = tuple_element_t<previousSubPassIndex, Tuple<RenderSubPass_t...>>;
			return tuple_element_t<findDependency<transitioningResourceIndex, typename PreviousSubPass::Dependencies>(), typename PreviousSubPass::DependencyStates>::value;
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
		ThreadLocal(GraphicsEngine& graphicsEngine) : subPassesThreadLocal{ (sizeof(typename RenderSubPass_t::ThreadLocal), graphicsEngine)... } {}

		/*
		Must be called from the first thread
		*/
		template<class ThreadResources>
		void startFrame(ThreadResources& threadResources, GraphicsEngine& graphicsEngine)
		{
			graphicsEngine.startFrame(&threadResources);
		}

		void update1After(GraphicsEngine& graphicsEngine, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature, unsigned int primaryThreadIndex)
		{
			if (primaryThreadIndex == 0u)
			{
				update1After<0u, sizeof...(RenderSubPass_t), true>(graphicsEngine, renderPass, rootSignature);
			}
			else
			{
				update1After<0u, sizeof...(RenderSubPass_t), false>(graphicsEngine, renderPass, rootSignature);
			}
		}

		template<class Executor>
		void update2(Executor& executor, RenderPass<RenderSubPass_t...>& renderPass, unsigned int threadCount, unsigned int primaryThreadIndex)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			if (primaryThreadIndex == 0u)
			{
				if (threadCount == 1u)
				{
					update2LastThread<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get(), threadCount);
				}
				else
				{
					update2<0u, subPassCount>(renderPass, renderPass.commandLists.get(), threadCount);
				}
			}
			else
			{
				if (threadCount == primaryThreadIndex + 1u)
				{
					update2LastThread<0u, subPassCount>(executor, renderPass, renderPass.commandLists.get() + primaryThreadIndex, threadCount);
				}
				else
				{
					update2<0u, subPassCount>(renderPass, renderPass.commandLists.get() + primaryThreadIndex, threadCount);
				}
			}
		}

		void present(unsigned int primaryThreadCount, GraphicsEngine& graphicsEngine, Window& window, RenderPass<RenderSubPass_t...>& renderPass)
		{
			unsigned int commandListCount = commandListPerThread(renderPass.mSubPasses) * primaryThreadCount;
			graphicsEngine.endFrame(window, renderPass.commandLists.get(), commandListCount);
		}
	private:
		unsigned int commandListPerThread(Tuple<RenderSubPass_t...>& subPasses)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			return commandListPerThread<0u, subPassCount>(subPasses);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start != end, unsigned int> commandListPerThread(Tuple<RenderSubPass_t...>& subPasses)
		{
			auto& subPass = get<start>(subPasses);
			if (subPass.isInView())
			{
				return subPass.commandListsPerFrame + commandListPerThread<start + 1u, end>(subPasses);
			}
			return commandListPerThread<start + 1u, end>(subPasses);
		}

		template<unsigned int start, unsigned int end>
		std::enable_if_t<start == end, unsigned int> commandListPerThread(Tuple<RenderSubPass_t...>&)
		{
			return 0u;
		}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<!(start != end), void> update1After(GraphicsEngine&, RenderPass<RenderSubPass_t...>&, ID3D12RootSignature*) {}

		template<unsigned int start, unsigned int end, bool firstThread>
		std::enable_if_t<start != end, void>
			update1After(GraphicsEngine& graphicsEngine, RenderPass<RenderSubPass_t...>& renderPass, ID3D12RootSignature* rootSignature)
		{
			auto& subPassLocal = get<start>(subPassesThreadLocal);
			auto& subPass = get<start>(renderPass.mSubPasses);
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
			const auto commandListsPerFrame = get<start>(renderPass.mSubPasses).commandListsPerFrame;
			auto& subPassLocal = get<start>(subPassesThreadLocal);
			auto& subPass = get<start>(renderPass.mSubPasses);
			if (subPass.isInView())
			{
				subPassLocal.update2(commandLists, numThreads);
			}
			update2<start + 1u, end>(renderPass, commandLists, numThreads);
		}


		template<unsigned int start, unsigned int end, class ThreadResources>
		std::enable_if_t<start == end, void>
			update2LastThread(ThreadResources&, RenderPass<RenderSubPass_t...>&, ID3D12CommandList**,
				unsigned int) {}

		template<unsigned int start, unsigned int end, class ThreadResources>
		std::enable_if_t<start != end, void>
			update2LastThread(ThreadResources& threadResources, RenderPass<RenderSubPass_t...>& renderPass, ID3D12CommandList** commandLists,
				unsigned int numThreads)
		{
			constexpr auto subPassCount = sizeof...(RenderSubPass_t);
			auto& subPass = get<start>(renderPass.mSubPasses);
			if (subPass.isInView())
			{
				auto& subPassLocal = get<start>(subPassesThreadLocal);
				subPassLocal.update2LastThread(commandLists, numThreads, subPass, threadResources);
			}
			update2LastThread<start + 1u, end>(threadResources, renderPass, commandLists, numThreads);
		}
	};
};