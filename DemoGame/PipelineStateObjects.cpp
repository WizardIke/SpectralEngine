#include "PipelineStateObjects.h"
#include <d3d12.h>
#include <IOCompletionQueue.h>
#include <makeArray.h>

#include "Generated/Resources/PipelineStateObjects/BasicPso.h"
#include "Generated/Resources/PipelineStateObjects/CopyPso.h"
#include "Generated/Resources/PipelineStateObjects/DirectionalLightPso.h"
#include "Generated/Resources/PipelineStateObjects/DirectionalLightVtPso.h"
#include "Generated/Resources/PipelineStateObjects/DirectionalLightVtTwoSidedPso.h"
#include "Generated/Resources/PipelineStateObjects/FirePso.h"
#include "Generated/Resources/PipelineStateObjects/GlassPso.h"
#include "Generated/Resources/PipelineStateObjects/PointLightPso.h"
#include "Generated/Resources/PipelineStateObjects/TextPso.h"
#include "Generated/Resources/PipelineStateObjects/VtDebugDrawPso.h"
#include "Generated/Resources/PipelineStateObjects/VtFeedbackPso.h"
#include "Generated/Resources/PipelineStateObjects/VtFeedbackWithNormalsPso.h"
#include "Generated/Resources/PipelineStateObjects/VtFeedbackWithNormalsTwoSidedPso.h"
#include "Generated/Resources/PipelineStateObjects/WaterNoReflectionTexturePso.h"
#include "Generated/Resources/PipelineStateObjects/WaterWithReflectionTexturePso.h"


PipelineStateObjects::PipelineStateObjects(AsynchronousFileManager& asynchronousFileManager, ID3D12Device& grphicsDevice, RootSignatures& rootSignatures, PipelineLoader& pipelineLoader
#ifndef NDEBUG
	, bool isWarp
#endif
)
{
	new(&pipelineLoader.impl) PipelineLoaderImpl(
		{ &PipelineStateObjectDescs::TextPso::desc, &PipelineStateObjectDescs::DirectionalLightPso::desc, &PipelineStateObjectDescs::DirectionalLightVtPso::desc, &PipelineStateObjectDescs::DirectionalLightVtTwoSidedPso::desc, &PipelineStateObjectDescs::PointLightPso::desc,
		&PipelineStateObjectDescs::WaterWithReflectionTexturePso::desc, &PipelineStateObjectDescs::WaterNoReflectionTexturePso::desc, &PipelineStateObjectDescs::GlassPso::desc, &PipelineStateObjectDescs::BasicPso::desc, &PipelineStateObjectDescs::FirePso::desc,
		&PipelineStateObjectDescs::CopyPso::desc, &PipelineStateObjectDescs::VtFeedbackPso::desc, &PipelineStateObjectDescs::VtFeedbackWithNormalsPso::desc, &PipelineStateObjectDescs::VtFeedbackWithNormalsTwoSidedPso::desc, &PipelineStateObjectDescs::VtDebugDrawPso::desc },
		{ &text, &directionalLight, &directionalLightVt, &directionalLightVtTwoSided, &pointLight, &waterWithReflectionTexture, &waterNoReflectionTexture, &glass, &basic, &fire, &copy, &vtFeedback, &vtFeedbackWithNormals, &vtFeedbackWithNormalsTwoSided, &vtDebugDraw },
		asynchronousFileManager, grphicsDevice, pipelineLoader, *rootSignatures.rootSignature
#ifndef NDEBUG
		, isWarp
#endif
	);
}

PipelineStateObjects::PipelineLoaderImpl::PipelineLoaderImpl(GraphicsPipelineStateDesc* const(&graphicsPipelineStateDescs)[numberOfComponents], D3D12PipelineState* const(&output)[numberOfComponents],
	AsynchronousFileManager& asynchronousFileManager, ID3D12Device& grphicsDevice, PipelineLoader& pipelineLoader, ID3D12RootSignature& rootSignature
#ifndef NDEBUG
	, bool isWarp
#endif
) :
	psoLoaders{ makeArray<numberOfComponents>([&](std::size_t i)
		{
			return ShaderLoader{*graphicsPipelineStateDescs[i], asynchronousFileManager, grphicsDevice, componentLoaded, &pipelineLoader, *output[i]};
		}) }
{
	for (auto graphicsPipelineStateDesc : graphicsPipelineStateDescs)
	{
		graphicsPipelineStateDesc->pRootSignature = &rootSignature;
#ifndef NDEBUG
		if (isWarp)
		{
			graphicsPipelineStateDesc->Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
		}
#endif
	}
	for (auto& loader : psoLoaders)
	{
		PsoLoader::loadPsoWithVertexAndPixelShaders(loader, asynchronousFileManager);
	}
}

void PipelineStateObjects::PipelineLoaderImpl::componentLoaded(PsoLoader::PsoWithVertexAndPixelShaderRequest& request1, D3D12PipelineState pso, void* tr)
{
	auto& request = static_cast<ShaderLoader&>(request1);
	request.piplineStateObject = std::move(pso);
	auto pipelineLoader = request.pipelineLoader;
	if(pipelineLoader->impl.numberOfcomponentsLoaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
	{
		pipelineLoader->impl.~PipelineLoaderImpl();
		pipelineLoader->loadingFinished(*pipelineLoader, tr);
	}
}