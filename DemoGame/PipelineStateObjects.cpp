#include "PipelineStateObjects.h"
#include <d3d12.h>
#include <IOCompletionQueue.h>
#include "GlobalResources.h"
#include "ThreadResources.h"

namespace
{
	using ShaderInfo = PipelineStateObjects::PipelineLoaderImpl::ShaderInfo;
	using ShaderRequestVP = PipelineStateObjects::ShaderRequestVP;
	ShaderInfo textShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return 
		{
			L"../DemoGame/CompiledShaders/TextVS.cso",
			L"../DemoGame/CompiledShaders/TextPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = TRUE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.text = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.text->SetName(L"Text");
#endif
		}
		};
	}

	ShaderInfo directionalLightShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/LightVS.cso",
			L"../DemoGame/CompiledShaders/DirectionalLightPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.directionalLight = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.directionalLight->SetName(L"DirectionalLight");
#endif
		}
		};
	}

	ShaderInfo directionalLightVtShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			 L"../DemoGame/CompiledShaders/LightVS.cso",
			 L"../DemoGame/CompiledShaders/DirectionalLightVtPS.cso",
			 rootSignature,
			 [](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.directionalLightVt = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.directionalLightVt->SetName(L"directionalLightVt");
#endif
		}
		};
	}

	ShaderInfo directionalLightVtTwoSidedShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/LightVS.cso",
			L"../DemoGame/CompiledShaders/DirectionalLightVtPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.directionalLightVtTwoSided = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.directionalLightVtTwoSided->SetName(L"directionalLightVtTwoSided");
#endif
		}
		};
	}

	ShaderInfo pointLightShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/LightVS.cso",
			L"../DemoGame/CompiledShaders/PointLightPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.pointLight = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.pointLight->SetName(L"pointLight");
#endif
		}
		};
	}

	ShaderInfo waterWithReflectionTextureShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/WaterVS.cso",
			L"../DemoGame/CompiledShaders/WaterWithReflectionTexturePS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.waterWithReflectionTexture = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.waterWithReflectionTexture->SetName(L"waterWithReflectionTexture");
#endif
		}
		};
	}

	ShaderInfo waterNoReflectionTextureShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/WaterVS.cso",
			L"../DemoGame/CompiledShaders/WaterNoReflectionTexturePS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.waterNoReflectionTexture = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.waterNoReflectionTexture->SetName(L"waterNoReflectionTexture");
#endif
		}
		};
	}

	ShaderInfo glassShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/GlassVS.cso",
			L"../DemoGame/CompiledShaders/GlassPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.glass = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.glass->SetName(L"glass");
#endif
		}
		};
	}

	ShaderInfo basicShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/BasicVS.cso",
			L"../DemoGame/CompiledShaders/BasicPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.basic = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.basic->SetName(L"basic");
#endif
		}
		};
	}

	ShaderInfo vtFeedbackShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/BasicVS.cso",
			L"../DemoGame/CompiledShaders/VtFeedbackPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_UINT;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtFeedback = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtFeedback->SetName(L"Virtual texture feedback");
#endif
		}
		};
	}

	ShaderInfo vtFeedbackWithNormalsShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/BasicVS.cso",
			L"../DemoGame/CompiledShaders/VtFeedbackPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_UINT;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtFeedbackWithNormals = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtFeedbackWithNormals->SetName(L"Virtual texture feedback with normals");
#endif
		}
		};
	}

	ShaderInfo vtFeedbackWithNormalsTwoSidedShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/BasicVS.cso",
			L"../DemoGame/CompiledShaders/VtFeedbackPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_UINT;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtFeedbackWithNormalsTwoSided = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtFeedbackWithNormalsTwoSided->SetName(L"Virtual texture feedback with normals two sided");
#endif
		}
		};
	}

	ShaderInfo vtDebugDrawShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/TexturedQuadVS.cso",
			L"../DemoGame/CompiledShaders/VtDebugDrawPs.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtDebugDraw = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtDebugDraw->SetName(L"virtual texture feedback debug draw");
#endif
		}
		};
	}

	ShaderInfo copyShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/PositionOnlyVS.cso",
			L"../DemoGame/CompiledShaders/CopyPS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = FALSE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.copy = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.copy->SetName(L"Copy");
#endif
		}
		};
	}

	ShaderInfo fireShaderInfo(ID3D12RootSignature* rootSignature)
	{
		return
		{
			L"../DemoGame/CompiledShaders/FireVS.cso",
			L"../DemoGame/CompiledShaders/FirePS.cso",
			rootSignature,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device)
		{
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.fire = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.fire->SetName(L"fire");
#endif
		}
		};
	}
}

PipelineStateObjects::PipelineStateObjects(ThreadResources& threadResources, GlobalResources& globalResources, PipelineLoader& pipelineLoader)
{
	ID3D12RootSignature* const rootSignature = globalResources.rootSignatures.rootSignature;

	new(&pipelineLoader.impl) PipelineLoaderImpl(
		{
			textShaderInfo(rootSignature),
			directionalLightShaderInfo(rootSignature),
			directionalLightVtShaderInfo(rootSignature),
			directionalLightVtTwoSidedShaderInfo(rootSignature),
			pointLightShaderInfo(rootSignature),
			waterWithReflectionTextureShaderInfo(rootSignature),
			waterNoReflectionTextureShaderInfo(rootSignature),
			glassShaderInfo(rootSignature),
			basicShaderInfo(rootSignature),
			vtFeedbackShaderInfo(rootSignature),
			vtFeedbackWithNormalsShaderInfo(rootSignature),
			vtFeedbackWithNormalsTwoSidedShaderInfo(rootSignature),
			vtDebugDrawShaderInfo(rootSignature),
			copyShaderInfo(rootSignature),
			fireShaderInfo(rootSignature)
		}, threadResources, globalResources, pipelineLoader);
}

void PipelineStateObjects::ShaderRequestVP::freeRequestMemory(AsynchronousFileManager::ReadRequest& request1, void*, void*)
{
	ShaderRequestVP& request = *static_cast<RequestHelper&>(request1).request;
	if(request.numberOfComponentsReadyToDelete.fetch_add(1u, std::memory_order::memory_order_acq_rel) == (numberOfComponents - 1u))
	{
		request.deleteRequest(request);
	}
}

void PipelineStateObjects::ShaderRequestVP::shaderFileLoaded(ThreadResources& threadResources, GlobalResources& globalResources)
{
	if(numberOfComponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel) == (numberOfComponents - 1u))
	{
		loadDescription(vertexShaderData, vertexHelper.end, pixelShaderData, pixelHelper.end,
			globalResources.pipelineStateObjects, rootSignature, globalResources.graphicsEngine.graphicsDevice);
		loadingFinished(*this, threadResources, globalResources);
		globalResources.asynchronousFileManager.discard(&vertexHelper, threadResources, globalResources);
		globalResources.asynchronousFileManager.discard(&pixelHelper, threadResources, globalResources);
	}
}

PipelineStateObjects::ShaderRequestVP::ShaderRequestVP(ThreadResources& threadResources, GlobalResources& globalResources, const wchar_t* vertexFile, const wchar_t* pixelFile, ID3D12RootSignature* rootSignature1,
	void(*loadingFinished1)(ShaderRequestVP& request, ThreadResources& threadResources, GlobalResources& globalResources),
	void(*loadDescription1)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
		PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device),
	void(*deleteRequest1)(ShaderRequestVP& request)) :
	loadDescription(loadDescription1),
	loadingFinished(loadingFinished1),
	deleteRequest(deleteRequest1)
{
	vertexHelper.request = this;
	pixelHelper.request = this;
	rootSignature = rootSignature1;

	AsynchronousFileManager& fileManager = globalResources.asynchronousFileManager;
	IOCompletionQueue& ioCompletionQueue = globalResources.ioCompletionQueue;

	File vsFile = fileManager.openFileForReading<GlobalResources>(ioCompletionQueue, vertexFile);
	RequestHelper& request1 = vertexHelper;
	request1.filename = vertexFile;
	request1.file = vsFile;
	request1.start = 0u;
	request1.end = vsFile.size();
	request1.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* executor, void* sharedResources, const unsigned char* data)
	{
		request.file.close();

		ShaderRequestVP& request1 = *static_cast<RequestHelper&>(request).request;
		request1.vertexShaderData = data;
		request1.shaderFileLoaded(*static_cast<ThreadResources*>(executor), *static_cast<GlobalResources*>(sharedResources));
	};
	request1.deleteReadRequest = freeRequestMemory;
	fileManager.readFile(&request1, threadResources, globalResources);

	File psFile = fileManager.openFileForReading<GlobalResources>(ioCompletionQueue, pixelFile);
	RequestHelper& request2 = pixelHelper;
	request2.filename = pixelFile;
	request2.file = psFile;
	request2.start = 0u;
	request2.end = psFile.size();
	request2.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* executor, void* sharedResources, const unsigned char* data)
	{
		request.file.close();

		ShaderRequestVP& request1 = *static_cast<RequestHelper&>(request).request;
		request1.pixelShaderData = data;
		request1.shaderFileLoaded(*static_cast<ThreadResources*>(executor), *static_cast<GlobalResources*>(sharedResources));
	};
	request2.deleteReadRequest = freeRequestMemory;
	fileManager.readFile(&request2, threadResources, globalResources);
}

PipelineStateObjects::PipelineLoaderImpl::PipelineLoaderImpl(const ShaderInfo(&shadowInfos)[numberOfComponents], ThreadResources& threadResources, GlobalResources& globalResources, PipelineLoader& pipelineLoader) :
	shaderLoaders([&](std::size_t i, ShaderLoader& shaderLoader)
{
	new(&shaderLoader) ShaderLoader(threadResources, globalResources, shadowInfos[i].vertexShader, shadowInfos[i].pixelShader, shadowInfos[i].rootSignature, componentLoaded, shadowInfos[i].loadDescription, freeComponent, &pipelineLoader);
})
{}

void PipelineStateObjects::PipelineLoaderImpl::componentLoaded(ShaderRequestVP& request1, ThreadResources& threadResources, GlobalResources& globalResources)
{
	auto& request = static_cast<ShaderLoader&>(request1);
	auto pipelineLoader = request.pipelineLoader;
	if(pipelineLoader->impl.numberOfcomponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel) == (numberOfComponents - 1u))
	{
		pipelineLoader->loadingFinished(*pipelineLoader, threadResources, globalResources);
	}
}

void PipelineStateObjects::PipelineLoaderImpl::freeComponent(ShaderRequestVP& request1)
{
	auto pipelineLoader = static_cast<ShaderLoader&>(request1).pipelineLoader;
	if(pipelineLoader->impl.numberOfComponentsReadyToDelete.fetch_add(1u, std::memory_order::memory_order_acq_rel) == (numberOfComponents - 1u))
	{
		pipelineLoader->impl.~PipelineLoaderImpl();
		pipelineLoader->deleteRequest(*pipelineLoader);
	}
}

PipelineStateObjects::PipelineLoaderImpl::ShaderLoader::ShaderLoader(ThreadResources& threadResources, GlobalResources& globalResources, const wchar_t* vertexFile, const wchar_t* pixelFile, ID3D12RootSignature* rootSignature1,
	void(*loadingFinished1)(ShaderRequestVP& request, ThreadResources& threadResources, GlobalResources& globalResources),
	void(*loadDescription1)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
		PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device),
	void(*deleteRequest1)(ShaderRequestVP& request), PipelineLoader* pipelineLoader) :
	ShaderRequestVP(threadResources, globalResources, vertexFile, pixelFile, rootSignature1, loadingFinished1, loadDescription1, deleteRequest1),
	pipelineLoader(pipelineLoader) {}