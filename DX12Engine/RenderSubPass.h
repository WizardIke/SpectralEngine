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

//use std::tuple<std::integral_constant<unsigned int, value>, ...> for dependencies
//use std::tuple<std::integral_constant<D3D12_RESOURCE_STATES, value>, ...> for dependencyStates
template<class Camera, D3D12_RESOURCE_STATES state1, class Dependencies_t, class DependencyStates_t, unsigned int commandListsPerFrame1>
class RenderSubPass
{
public:
	using Dependencies = Dependencies_t;
	constexpr static auto state = state1;
	using DependencyStates = DependencyStates_t;
	constexpr static bool empty = false;
	constexpr static auto commandListsPerFrame = commandListsPerFrame1;
	std::vector<Camera*> cameras;

	unsigned int cameraCount() const noexcept
	{
		return (unsigned int)cameras.size();
	}

	bool isInView(const Frustum& frustum) const
	{
		for (auto& camera : cameras)
		{
			if (camera->isInView(frustum))
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
			std::array<D3D12GraphicsCommandListPointer, commandListsPerFrame> commandLists;
		};
		std::array<PerFrameData, frameBufferCount> perFrameDatas;

		void bindRootArguments(ID3D12RootSignature* rootSignature, ID3D12DescriptorHeap* mainDescriptorHeap)
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
		ThreadLocal(const ThreadLocal&) = delete;
	public:
		ThreadLocal(BaseExecutor* const executor)
		{
			auto sharedResources = executor->sharedResources;
			auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			auto i = 0u;
			for (auto& perFrameData : perFrameDatas)
			{
				auto* commandList = &perFrameData.commandLists[0u];
				for (auto& allocator : perFrameData.commandAllocators)
				{

					new(&allocator) D3D12CommandAllocator(sharedResources->graphicsEngine.graphicsDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
					new(commandList) D3D12GraphicsCommandListPointer(sharedResources->graphicsEngine.graphicsDevice, 0u, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr);
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

		void update1After(BaseExecutor* const executor, RenderSubPass<Camera, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame>& renderSubPass,
			ID3D12RootSignature* rootSignature)
		{
			auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			resetCommandLists(frameIndex);
			bindRootArguments(rootSignature, executor->sharedResources->graphicsEngine.mainDescriptorHeap);

			auto& mainFrustum = executor->sharedResources->mainFrustum;
			for (auto& camera : renderSubPass.cameras)
			{
				if (camera->isInView(mainFrustum))
				{
					camera->bind(*executor->sharedResources, frameIndex, &currentData->commandLists[0u].get(),
						&currentData->commandLists.data()[currentData->commandLists.size()].get());
				}
			}
		}

		void update1AfterFirstThread(BaseExecutor* const executor, RenderSubPass<Camera, state1, Dependencies_t, DependencyStates_t, commandListsPerFrame>& renderSubPass,
			ID3D12RootSignature* rootSignature, uint32_t barrierCount, const D3D12_RESOURCE_BARRIER* barriers)
		{
			auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			resetCommandLists(frameIndex);
			bindRootArguments(rootSignature, executor->sharedResources->graphicsEngine.mainDescriptorHeap);

			currentData->commandLists[0u]->ResourceBarrier(barrierCount, barriers);

			auto& mainFrustum = executor->sharedResources->mainFrustum;
			for (auto& camera : renderSubPass.cameras)
			{
				if (camera->isInView(mainFrustum))
				{
					camera->bind(*executor->sharedResources, frameIndex, &currentData->commandLists[0u].get(),
						&currentData->commandLists.data()[currentData->commandLists.size()].get());
				}
			}
		}

		void update2(ID3D12CommandList** commandLists)
		{
			for (ID3D12GraphicsCommandList* commandList : currentData->commandLists)
			{
				auto result = commandList->Close();
				if (FAILED(result)) throw HresultException(result);
			}

			for (ID3D12GraphicsCommandList* commandList : currentData->commandLists)
			{
				*commandLists = commandList;
				++commandLists;
			}
		}

		ID3D12GraphicsCommandList* lastCommandList()
		{
			return currentData->commandLists[commandListsPerFrame - 1u];
		}

		PerFrameData* currentData;
		constexpr static bool empty = false;
	};
};

template<class Camera, D3D12_RESOURCE_STATES state1, class Dependencies_t, class DependencyStates_t>
class RenderSubPass<Camera, state1, Dependencies_t, DependencyStates_t, 0u>
{
public:
	using Dependencies = Dependencies_t;
	constexpr static auto state = state1;
	using DependencyStates = DependencyStates_t;
	constexpr static bool empty = true;

	static void update2LastThread(ID3D12GraphicsCommandList* lastCommandList, uint32_t barrierCount, const D3D12_RESOURCE_BARRIER* barriers)
	{
		lastCommandList->ResourceBarrier(barrierCount, barriers);
	}

	static bool isInView(const Frustum& frustum)
	{
		return true;
	}

	constexpr static unsigned int cameraCount() noexcept
	{
		return 0u;
	}

	
	class ThreadLocal
	{
	public:
		constexpr ThreadLocal(BaseExecutor* const executor) {}
		constexpr static bool empty = true;
	};
};