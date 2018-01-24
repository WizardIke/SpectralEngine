#pragma once
#include "D3D12CommandAllocator.h"
#include "frameBufferCount.h"
#include "D3D12GraphicsCommandList.h"
#include "for_each.h"
#include "Frustum.h"
#include <vector>
#include <array>
#include "SharedResources.h"
#include "BaseExecutor.h"
#include "Range.h"
#include "../Array/ResizingArray.h"
#include "ReflectionCamera.h"

//use std::tuple<std::integral_constant<unsigned int, value>, ...> for dependencies
//use std::tuple<std::integral_constant<D3D12_RESOURCE_STATES, value>, ...> for dependencyStates
template<class Camera_t, D3D12_RESOURCE_STATES state1, class Dependencies_t, class DependencyStates_t, unsigned int commandListsPerFrame1, D3D12_RESOURCE_STATES stateAfter1 = state1>
class RenderSubPass
{
	std::vector<Camera_t*> mCameras;
public:
	using Camera = Camera_t;
	using CameraIterator = typename std::vector<Camera*>::iterator;
	using ConstCameraIterator = typename std::vector<Camera*>::const_iterator;
	using Dependencies = Dependencies_t;
	constexpr static auto state = state1;
	constexpr static auto stateAfter = stateAfter1;
	using DependencyStates = DependencyStates_t;
	constexpr static auto commandListsPerFrame = commandListsPerFrame1;
	constexpr static bool isPresentSubPass = false;

	RenderSubPass() noexcept {}
	RenderSubPass(RenderSubPass<Camera_t, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame1, stateAfter1>&& other) noexcept : mCameras(std::move(other.mCameras)) {}

	void operator=(RenderSubPass<Camera_t, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame1, stateAfter1>&& other) noexcept
	{
		mCameras = std::move(other.mCameras);
	}
	
	Range<CameraIterator> cameras() noexcept
	{
		return { mCameras.begin(), mCameras.end()};
	}

	Range<ConstCameraIterator> cameras() const noexcept
	{
		return { mCameras.begin(), mCameras.end() };
	}

	unsigned int cameraCount() const noexcept
	{
		return (unsigned int)mCameras.size();
	}

	template<class RenderPass>
	void addCamera(SharedResources& sharedResources, RenderPass& renderPass, Camera* const camera)
	{
		std::lock_guard<decltype(sharedResources.syncMutex)> lock(sharedResources.syncMutex);
		mCameras.push_back(camera);
		renderPass.updateBarrierCount();
	}

	void removeCamera(SharedResources& sharedResources, Camera* const camera) noexcept
	{
		std::lock_guard<decltype(sharedResources.syncMutex)> lock(sharedResources.syncMutex);
		auto cam = std::find(mCameras.begin(), mCameras.end(), camera);
		std::swap(*cam, *(mCameras.end() - 1u));
		mCameras.pop_back();
	}

	bool isInView(SharedResources& sharedResources) const noexcept
	{
		for (auto& camera : mCameras)
		{
			if (camera->isInView(sharedResources))
			{
				return true;
			}
		}
		return false;
	}

	class ThreadLocal
	{
		struct PerFrameData
		{
			std::array<D3D12CommandAllocator, commandListsPerFrame> commandAllocators;
			std::array<D3D12GraphicsCommandList, commandListsPerFrame> commandLists;
		};
		std::array<PerFrameData, frameBufferCount> perFrameDatas;

		void bindRootArguments(ID3D12RootSignature* rootSignature, ID3D12DescriptorHeap* mainDescriptorHeap) noexcept
		{
			auto textureDescriptorTable = mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			for (ID3D12GraphicsCommandList* commandList : currentData->commandLists)
			{
				commandList->SetGraphicsRootSignature(rootSignature);
				commandList->SetDescriptorHeaps(1u, &mainDescriptorHeap);
				commandList->SetGraphicsRootDescriptorTable(4u, textureDescriptorTable);
			}
		}

		void resetCommandLists(uint32_t frameIndex)
		{
			currentData = &perFrameDatas[frameIndex];
			ID3D12GraphicsCommandList** commandList = &currentData->commandLists[0u].get();
			for (ID3D12CommandAllocator* commandAllocator : currentData->commandAllocators)
			{
				auto result = commandAllocator->Reset();
				if (FAILED(result)) throw HresultException(result);

				result = (*commandList)->Reset(commandAllocator, nullptr);
				if (FAILED(result)) throw HresultException(result);
				++commandList;
			}
		}
		ThreadLocal() = delete;
	public:
		ThreadLocal(ThreadLocal&& other) noexcept
		{
			auto end = perFrameDatas.end();
			for (auto perFrameData1 = perFrameDatas.begin(), perFrameData2 = other.perFrameDatas.begin(); perFrameData1 != end; ++perFrameData1, ++perFrameData2)
			{
				auto& commandAllocators1 = perFrameData1->commandAllocators;
				auto& commandAllocators2 = perFrameData2->commandAllocators;
				auto end2 = commandAllocators1.end();
				for (auto commandAllocator1 = commandAllocators1.begin(), commandAllocator2 = commandAllocators2.begin(); commandAllocator1 != end2; ++commandAllocator1, ++commandAllocator2)
				{
					*commandAllocator1 = std::move(*commandAllocator2);
				}

				auto& commandLists1 = perFrameData1->commandLists;
				auto& commandLists2 = perFrameData2->commandLists;
				auto end3 = commandLists1.end();
				for (auto commandList1 = commandLists1.begin(), commandList2 = commandLists2.begin(); commandList1 != end3; ++commandList1, ++commandList2)
				{
					*commandList1 = std::move(*commandList2);
				}
			}
		}
		ThreadLocal(SharedResources& sharedResources)
		{
			auto frameIndex = sharedResources.graphicsEngine.frameIndex;
			auto i = 0u;
			for (auto& perFrameData : perFrameDatas)
			{
				auto* commandList = &perFrameData.commandLists[0u];
				for (auto& allocator : perFrameData.commandAllocators)
				{

					new(&allocator) D3D12CommandAllocator(sharedResources.graphicsEngine.graphicsDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
					new(commandList) D3D12GraphicsCommandList(sharedResources.graphicsEngine.graphicsDevice, 0u, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr);
					(*commandList)->Close();

#ifdef _DEBUG
					std::wstring name = L"Direct command allocator ";
					name += std::to_wstring(i);
					allocator->SetName(name.c_str());

					name = L"Direct command list ";
					name += std::to_wstring(i);
					(*commandList)->SetName(name.c_str());
#endif // _DEBUG
					++commandList;
				}
				++i;
			}
			currentData = &perFrameDatas[frameIndex];
		}

		void operator=(ThreadLocal&& other) noexcept
		{
			this->~ThreadLocal();
			new(this) ThreadLocal(std::move(other));
		}

		void update1After(SharedResources& sharedResources, RenderSubPass<Camera, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame, stateAfter1>& renderSubPass,
			ID3D12RootSignature* rootSignature)
		{
			auto frameIndex = sharedResources.graphicsEngine.frameIndex;
			resetCommandLists(frameIndex);
			bindRootArguments(rootSignature, sharedResources.graphicsEngine.mainDescriptorHeap);
			auto cameras = renderSubPass.cameras();
			for (auto& camera : cameras)
			{
				if (camera->isInView(sharedResources))
				{
					camera->bind(sharedResources, &currentData->commandLists[0u].get(),
						&currentData->commandLists.data()[currentData->commandLists.size()].get());
				}
			}
		}

		void update1AfterFirstThread(SharedResources& sharedResources, RenderSubPass<Camera, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame, stateAfter1>& renderSubPass,
			ID3D12RootSignature* rootSignature, uint32_t barrierCount, const D3D12_RESOURCE_BARRIER* barriers)
		{
			auto frameIndex = sharedResources.graphicsEngine.frameIndex;
			resetCommandLists(frameIndex);
			bindRootArguments(rootSignature, sharedResources.graphicsEngine.mainDescriptorHeap);

			if(barrierCount != 0u) currentData->commandLists[0u]->ResourceBarrier(barrierCount, barriers);

			for (auto& camera : renderSubPass.cameras())
			{
				if (camera->isInView(sharedResources))
				{
					camera->bindFirstThread(sharedResources, &currentData->commandLists[0u].get(),
						&currentData->commandLists.data()[currentData->commandLists.size()].get());
				}
			}
		}

		void update1AfterFirstThread(SharedResources& sharedResources, RenderSubPass<Camera, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame, stateAfter1>& renderSubPass,
			ID3D12RootSignature* rootSignature)
		{
			auto frameIndex = sharedResources.graphicsEngine.frameIndex;
			resetCommandLists(frameIndex);
			bindRootArguments(rootSignature, sharedResources.graphicsEngine.mainDescriptorHeap);

			for (auto& camera : renderSubPass.cameras())
			{
				if (camera->isInView(sharedResources))
				{
					camera->bindFirstThread(sharedResources, &currentData->commandLists[0u].get(),
						&currentData->commandLists.data()[currentData->commandLists.size()].get());
				}
			}
		}

		void update2(ID3D12CommandList**& commandLists, unsigned int numThreads)
		{
			for (ID3D12GraphicsCommandList* commandList : currentData->commandLists)
			{
				auto result = commandList->Close();
				if (FAILED(result)) throw HresultException(result);
			}

			for (ID3D12GraphicsCommandList* commandList : currentData->commandLists)
			{
				*commandLists = commandList;
				commandLists += numThreads;
			}

		}

		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, RenderSubPass<Camera, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame, stateAfter1>& renderSubPass,
			BaseExecutor* executor, SharedResources& sharedResources)
		{
			update2(commandLists, numThreads);
		}

		ID3D12GraphicsCommandList* lastCommandList() noexcept
		{
			return currentData->commandLists[commandListsPerFrame - 1u];
		}

		ID3D12GraphicsCommandList* firstCommandList() noexcept
		{
			return currentData->commandLists[0];
		}

		PerFrameData* currentData;
	};
};

template<class Camera_t, D3D12_RESOURCE_STATES state1, class Dependencies_t, class DependencyStates_t, unsigned int commandListsPerFrame1, D3D12_RESOURCE_STATES stateAfter1 = state1>
class RenderMainSubPass : public RenderSubPass<Camera_t, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame1, stateAfter1>
{
public:
	class ThreadLocal : public RenderSubPass<Camera_t, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame1, stateAfter1>::ThreadLocal
	{
	public:
		ThreadLocal(SharedResources& sharedResources) : RenderSubPass<Camera_t, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame1, stateAfter1>::ThreadLocal(sharedResources) {}
		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, RenderMainSubPass<Camera_t, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame, stateAfter1>& renderSubPass,
			BaseExecutor* executor, SharedResources& sharedResources)
		{
			auto cameras = renderSubPass.cameras();
			auto camerasEnd = cameras.end();
			uint32_t barrierCount = 0u;
			D3D12_RESOURCE_BARRIER barriers[2];
			for (auto cam = cameras.begin(); cam != camerasEnd; ++cam)
			{
				assert(barrierCount != 2);
				auto camera = *cam;
				barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[barrierCount].Transition.pResource = camera->getImage();
				barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barriers[barrierCount].Transition.StateBefore = state;
				barriers[barrierCount].Transition.StateAfter = stateAfter;
				++barrierCount;
			}
			lastCommandList()->ResourceBarrier(barrierCount, barriers);
			update2(commandLists, numThreads);
		}
	};
};

template<class RenderSubPass_t>
class RenderSubPassGroup
{
	ResizingArray<RenderSubPass_t> mSubPasses;
	//std::vector<RenderSubPass_t> mSubPasses; //doesn't support move on realocation in some implementations
	using SubPasses = decltype(mSubPasses);

	template<class Camera, class CameraIterator, class SubPassIterator>
	class Iterator
	{
		friend class RenderSubPassGroup<RenderSubPass_t>;
		SubPassIterator iterator;
		SubPassIterator iteratorEnd;
		CameraIterator current;
		Iterator(SubPassIterator iterator, SubPassIterator iteratorEnd) : iterator(iterator), iteratorEnd(iteratorEnd), current() 
		{
			if (iterator != iteratorEnd)
			{
				current = iterator->cameras().begin();
			}
		}
	public:
		using value_type = Camera;
		using difference_type = typename std::iterator_traits<Camera*>::difference_type;
		using pointer = typename std::iterator_traits<Camera*>::pointer;
		using reference = typename std::iterator_traits<Camera*>::reference;
		using iterator_category = std::forward_iterator_tag;
		Iterator& operator++() noexcept
		{
			++current;
			auto end = iterator->cameras().end();
			while (current == end)
			{
				++iterator;
				if (iterator == iteratorEnd)
				{
					break;
				}
				current = iterator->cameras().begin();
				end = iterator->cameras().end();
			}
			return *this;
		}

		Iterator operator++(int) noexcept
		{
			Iterator ret{ iterator, current };
			++(*this);
			return ret;
		}

		Camera* operator*() noexcept
		{
			return *current;
		}

		Camera** operator->() noexcept
		{
			return &*current;
		}

		bool operator==(const SubPassIterator& other) noexcept
		{
			return iterator == other;
		}

		bool operator!=(const SubPassIterator& other) noexcept
		{
			return iterator != other;
		}
	};
public:
	using Camera = typename RenderSubPass_t::Camera;
	using SubPass = RenderSubPass_t;
	using Dependencies = typename RenderSubPass_t::Dependencies;
	constexpr static auto state = RenderSubPass_t::state;
	constexpr static auto stateAfter = RenderSubPass_t::stateAfter;
	using DependencyStates = typename RenderSubPass_t::DependencyStates;
	unsigned int commandListsPerFrame = 0u; // RenderSubPass_t::commandListsPerFrame * subPasses.size();
	using CameraIterator = Iterator<Camera, typename RenderSubPass_t::CameraIterator, typename SubPasses::iterator>;
	using ConstCameraIterator = Iterator<const Camera, typename RenderSubPass_t::ConstCameraIterator, typename SubPasses::const_iterator>;

	RenderSubPassGroup() = default;
	RenderSubPassGroup(RenderSubPassGroup&&) = default;

	Range<CameraIterator, typename SubPasses::iterator> cameras()
	{
		auto subPassBegin = mSubPasses.begin();
		auto subPassesEnd = mSubPasses.end();
		return { {subPassBegin, subPassesEnd}, subPassesEnd };
	}

	Range<ConstCameraIterator, typename SubPasses::const_iterator> cameras() const
	{
		auto subPassBegin = mSubPasses.begin();
		auto subPassesEnd = mSubPasses.end();
		return { { subPassBegin, subPassesEnd }, subPassesEnd };
	}

	unsigned int cameraCount() const noexcept
	{
		unsigned int count = 0u;
		for (auto& subPass : mSubPasses)
		{
			count += subPass.cameraCount();
		}
		return count;
	}

	Range<typename SubPasses::iterator> subPasses()
	{
		return {mSubPasses};
	}

	template<class RenderPass, class SubPasses>
	SubPass& addSubPass(SharedResources& sharedResources, RenderPass& renderPass, SubPasses subPasses)
	{
		std::lock_guard<decltype(sharedResources.syncMutex)> lock(sharedResources.syncMutex);
		auto& subPass = mSubPasses.emplace_back();
		auto end = subPasses.end();
		for (auto start = subPasses.begin(); start != end; ++start)
		{
			start->addSubPass(sharedResources);
		}
		renderPass.updateBarrierCount();

		commandListsPerFrame += subPass.commandListsPerFrame;
		return *(mSubPasses.end() - 1u);
	}

	template<class SubPasses>
	void removeSubPass(SharedResources& sharedResources, SubPass* subPass, SubPasses subPasses)
	{
		using std::swap; using std::find;
		std::lock_guard<decltype(sharedResources.syncMutex)> lock(sharedResources.syncMutex);
		auto pos = find_if(mSubPasses.begin(), mSubPasses.end(), [subPass](const SubPass& sub) {return subPass == &sub; });
		commandListsPerFrame -= pos->commandListsPerFrame;
		auto index = std::distance(mSubPasses.begin(), pos);

		auto nearlyEnd = (mSubPasses.end() - 1);
		if (pos != nearlyEnd) { swap(*pos, *(mSubPasses.end() - 1)); }
		mSubPasses.pop_back();

		auto end = subPasses.end();
		for (auto start = subPasses.begin(); start != end; ++start)
		{
			start->removeSubPass(sharedResources, index);
		}
	}

	bool isInView(SharedResources& sharedResources) const
	{
		for (auto& subPass : mSubPasses)
		{
			if (subPass.isInView(sharedResources))
			{
				return true;
			}
		}
		return false;
	}

	class ThreadLocal
	{
		std::vector<typename RenderSubPass_t::ThreadLocal> mSubPasses;
	public:
		ThreadLocal(SharedResources& sharedResources) {}

		using SubPass = typename RenderSubPass_t::ThreadLocal;

		Range<typename std::vector<SubPass>::iterator> subPasses()
		{
			return { mSubPasses.begin(), mSubPasses.end() };
		}

		void update1After(SharedResources& sharedResources, RenderSubPassGroup<RenderSubPass_t>& renderSubPassGruop,
			ID3D12RootSignature* rootSignature)
		{
			auto subPass = renderSubPassGruop.mSubPasses.begin();
			for (auto& subPassLocal : mSubPasses)
			{
				subPassLocal.update1After(sharedResources, *subPass, rootSignature);
				++subPass;
			}
		}

		void update1AfterFirstThread(SharedResources& sharedResources, RenderSubPassGroup<RenderSubPass_t>& renderSubPassGroup,
			ID3D12RootSignature* rootSignature, uint32_t barrierCount, const D3D12_RESOURCE_BARRIER* barriers)
		{
			auto subPass = renderSubPassGroup.mSubPasses.begin();
			auto subPassLocal = mSubPasses.begin();
			subPassLocal->update1AfterFirstThread(sharedResources, *subPass, rootSignature, barrierCount, barriers);
			++subPassLocal;
			++subPass;
			auto end = mSubPasses.end();
			for (; subPassLocal != end; ++subPassLocal)
			{
				subPassLocal->update1AfterFirstThread(sharedResources, *subPass, rootSignature);
				++subPass;
			}
		}

		void update2(ID3D12CommandList**& commandLists, unsigned int numThreads)
		{
			for (auto& subPassLocal : mSubPasses)
			{
				subPassLocal.update2(commandLists, numThreads);
			}
		}

		template<class Executor>
		void update2LastThread(ID3D12CommandList**& commandLists, unsigned int numThreads, RenderSubPassGroup<RenderSubPass_t>& renderSubPassGroup,
			Executor* executor, SharedResources& sharedResources)
		{
			const auto end = mSubPasses.end();
			auto subPass = renderSubPassGroup.mSubPasses.begin();
			for (auto& subPassLocal = mSubPasses.begin(); subPassLocal != end; ++subPassLocal, ++subPass)
			{
				subPassLocal->update2LastThread(commandLists, numThreads, *subPass, executor, sharedResources);
			}
		}

		ID3D12GraphicsCommandList* lastCommandList()
		{
			auto lastSubPass = mSubPasses.end() - 1u;
			return lastSubPass->lastCommandList();
		}

		void addSubPass(SharedResources& sharedResources)
		{
			//might need a mutex
			mSubPasses.emplace_back(sharedResources);
		}

		void removeSubPass(SharedResources& sharedResources, size_t index)
		{
			using std::swap;
			swap(mSubPasses[index], *(mSubPasses.end() - 1));
			mSubPasses.pop_back();
		}
	};
};