#pragma once
#include <D3D12PipelineState.h>
#include <atomic>
class ThreadResources;
class GlobalResources;

class PipelineStateObjects
{
public:
	class PipelineLoader
	{
		constexpr static unsigned int numberOfComponents = 15u;
		std::atomic<unsigned int> numberOfcomponentsLoaded = 0u;
		void(*loadingFinished)(PipelineStateObjects::PipelineLoader& pipelineLoader, ThreadResources& threadResources, GlobalResources& globalResources);
	public:
		PipelineLoader(void(*loadingFinished1)(PipelineStateObjects::PipelineLoader& pipelineLoader, ThreadResources& threadResources, GlobalResources& globalResources)) : loadingFinished(loadingFinished1) {}

		void componentLoaded(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			if(numberOfcomponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel) == (numberOfComponents - 1u))
			{
				loadingFinished(*this, threadResources, globalResources);
			}
		}
	};

	PipelineStateObjects(ThreadResources& threadResources, GlobalResources& globalResources, PipelineLoader* pipelineLoader);
	~PipelineStateObjects();

	D3D12PipelineState text;
	D3D12PipelineState directionalLight;
	D3D12PipelineState directionalLightVt;
	D3D12PipelineState directionalLightVtTwoSided;
	D3D12PipelineState pointLight;
	D3D12PipelineState waterWithReflectionTexture;
	D3D12PipelineState waterNoReflectionTexture;
	D3D12PipelineState glass;
	D3D12PipelineState basic;
	D3D12PipelineState fire;
	D3D12PipelineState copy;
	D3D12PipelineState vtFeedback;
	D3D12PipelineState vtFeedbackWithNormals;
	D3D12PipelineState vtFeedbackWithNormalsTwoSided;
	D3D12PipelineState vtDebugDraw;
};