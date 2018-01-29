#pragma once
#include <SharedResources.h>
#include <Font.h>
#include "RootSignatures.h"
#include "PipelineStateObjects.h"
#include "UserInterface/UserInterface.h"
#include "Areas.h"
#include "AmbientMusic.h"
#include <../Array/dynamicarray.h>
#include "MainExecutor.h"
#include "PrimaryExecutor.h"
#include "BackgroundExecutor.h"
#include "RenderPass.h"
#include <D3D12Resource.h>
#include <Range.h>
#include <VirtualTextureManager.h>
#include "InputHander.h"

class Assets : public SharedResources
{
	class ExecutorIterator
	{
		Executor* current;
		void(*next)(ExecutorIterator* iterator);
		Assets* assets;

		static void nextMainExecutors(ExecutorIterator* iterator)
		{
			iterator->current = iterator->assets->backgroundExecutors.begin();
			iterator->next = nextBackgroundExecutors;
		}

		static void nextBackgroundExecutors(ExecutorIterator* iterator)
		{
			iterator->current = reinterpret_cast<BackgroundExecutor*>(iterator->current) + 1u;
			if (iterator->current == iterator->assets->backgroundExecutors.end())
			{
				iterator->current = iterator->assets->primaryExecutors.begin();
				iterator->next = nextPrimaryExecutors;
			}
		}

		static void nextPrimaryExecutors(ExecutorIterator* iterator)
		{
			iterator->current = reinterpret_cast<PrimaryExecutor*>(iterator->current) + 1u;
		}
	public:
		using value_type = Executor;
		using difference_type = typename std::iterator_traits<Executor*>::difference_type;
		using pointer = typename std::iterator_traits<Executor*>::pointer;
		using reference = typename std::iterator_traits<Executor*>::reference;
		using iterator_category = std::forward_iterator_tag;
		ExecutorIterator(Assets* assets) : current(&assets->mainExecutor), next(nextMainExecutors), assets(assets) {}
		ExecutorIterator(const ExecutorIterator& other) = default;

		ExecutorIterator& operator++()
		{
			next(this);
			return *this;
		}

		bool operator!=(const Executor* other)
		{
			return current != other;
		}

		bool operator==(const Executor* other)
		{
			return current == other;
		}

		Executor& operator*()
		{
			return *current;
		}

		Executor* operator->()
		{
			return current;
		}
	};
public:
	Assets();
	~Assets();

	MainExecutor mainExecutor;
	InputHandler inputHandler;
	RootSignatures rootSignatures;
	PipelineStateObjects pipelineStateObjects; //Immutable
	VirtualTextureManager virtualTextureManager;
	D3D12Resource sharedConstantBuffer;
	uint8_t* constantBuffersCpuAddress;
	RenderPass1 renderPass;
	Font arial; //Immutable
	std::aligned_storage_t<sizeof(UserInterface), alignof(UserInterface)> mUserInterface;
	UserInterface& userInterface() { return *(UserInterface*)&mUserInterface; }
	Areas areas;
	AmbientMusic ambientMusic;
	MainCamera mainCamera;

	D3D12Resource warpTexture;
	unsigned int warpTextureDescriptorIndex;
	D3D12DescriptorHeap warpTextureCpuDescriptorHeap;

	DynamicArray<BackgroundExecutor> backgroundExecutors;
	DynamicArray<PrimaryExecutor> primaryExecutors;

	Range<ExecutorIterator, Executor*> executors()
	{
		return { {this},  primaryExecutors.end()};
	}

	void update(BaseExecutor* const executor);
	void start(BaseExecutor* executor);
};