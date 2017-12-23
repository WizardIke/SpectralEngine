#include "PipelineStateObjects.h"
#include <HresultException.h>
#include <d3dcompiler.h>
#include <d3d12.h>
#include "RootSignatures.h"
#include <memory>

PipelineStateObjects::PipelineStateObjects(ID3D12Device* const device, RootSignatures& rootSignatures)
{
	ID3DBlob* textVS;
	ID3DBlob* textPS;

	auto hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/TextVS.cso", &textVS);
	if (FAILED(hr)) throw false;
	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/TextPS.cso", &textPS);
	if (FAILED(hr)) throw false;

	D3D12_INPUT_ELEMENT_DESC textInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;

	PSODesc.pRootSignature = rootSignatures.rootSignature;
	PSODesc.VS.pShaderBytecode = textVS->GetBufferPointer();
	PSODesc.VS.BytecodeLength = textVS->GetBufferSize();
	PSODesc.PS.pShaderBytecode = textPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = textPS->GetBufferSize();
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
	PSODesc.InputLayout.NumElements = sizeof(textInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	PSODesc.InputLayout.pInputElementDescs = textInputLayout;
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

	new(&text) D3D12PipelineState(device, PSODesc);


	ID3DBlob* lightVS;
	ID3DBlob* directionalLightPS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/LightVS.cso", &lightVS);
	if (FAILED(hr)) throw false;
	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/DirectionalLightPS.cso", &directionalLightPS);
	if (FAILED(hr)) throw false;

	D3D12_INPUT_ELEMENT_DESC LightInputLayout[] =
	{
		{ "POSITION", 0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
		{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
		{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
	};

	PSODesc.VS.pShaderBytecode = lightVS->GetBufferPointer();
	PSODesc.VS.BytecodeLength = lightVS->GetBufferSize();
	PSODesc.PS.pShaderBytecode = directionalLightPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = directionalLightPS->GetBufferSize();
	PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
	PSODesc.InputLayout.NumElements = sizeof(LightInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	PSODesc.InputLayout.pInputElementDescs = LightInputLayout;

	new(&directionalLight) D3D12PipelineState(device, PSODesc);


	ID3DBlob* pointLightPS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/PointLightPS.cso", &pointLightPS);
	if (FAILED(hr)) throw false;

	PSODesc.PS.pShaderBytecode = pointLightPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = pointLightPS->GetBufferSize();

	new(&pointLight) D3D12PipelineState(device, PSODesc);


	ID3DBlob* waterVS;
	ID3DBlob* waterReflectionsPS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/WaterVS.cso", &waterVS);
	if (FAILED(hr)) throw false;
	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/WaterWithReflectionTexturePS.cso", &waterReflectionsPS);
	if (FAILED(hr)) throw false;

	const D3D12_INPUT_ELEMENT_DESC texturedInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	PSODesc.InputLayout.NumElements = sizeof(texturedInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	PSODesc.InputLayout.pInputElementDescs = texturedInputLayout;
	PSODesc.VS.pShaderBytecode = waterVS->GetBufferPointer();
	PSODesc.VS.BytecodeLength = waterVS->GetBufferSize();
	PSODesc.PS.pShaderBytecode = waterReflectionsPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = waterReflectionsPS->GetBufferSize();

	new(&waterWithReflectionTexture) D3D12PipelineState(device, PSODesc);


	ID3DBlob* waterNoReflectionsPS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/WaterNoReflectionTexturePS.cso", &waterNoReflectionsPS);

	PSODesc.PS.pShaderBytecode = waterNoReflectionsPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = waterNoReflectionsPS->GetBufferSize();

	new(&waterNoReflectionTexture) D3D12PipelineState(device, PSODesc);


	ID3DBlob* basicVS;
	ID3DBlob* glassPS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/BasicVS.cso", &basicVS);
	if (FAILED(hr)) throw false;
	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/GlassPS.cso", &glassPS);
	if (FAILED(hr)) throw false;

	PSODesc.VS.pShaderBytecode = basicVS->GetBufferPointer();
	PSODesc.VS.BytecodeLength = basicVS->GetBufferSize();
	PSODesc.PS.pShaderBytecode = glassPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = glassPS->GetBufferSize();

	new(&glass) D3D12PipelineState(device, PSODesc);


	ID3DBlob* basicPS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/BasicPS.cso", &basicPS);
	if (FAILED(hr)) throw false;

	PSODesc.PS.pShaderBytecode = basicPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = basicPS->GetBufferSize();

	new(&basic) D3D12PipelineState(device, PSODesc);


	ID3DBlob* copyVS;
	ID3DBlob* copyPS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/CopyVS.cso", &copyVS);
	if (FAILED(hr)) throw false;
	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/CopyPS.cso", &copyPS);
	if (FAILED(hr)) throw false;

	PSODesc.VS.pShaderBytecode = copyVS->GetBufferPointer();
	PSODesc.VS.BytecodeLength = copyVS->GetBufferSize();
	PSODesc.PS.pShaderBytecode = copyPS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = copyPS->GetBufferSize();

	D3D12_INPUT_ELEMENT_DESC copyInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	PSODesc.InputLayout.NumElements = sizeof(copyInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	PSODesc.InputLayout.pInputElementDescs = copyInputLayout;
	PSODesc.DepthStencilState.DepthEnable = FALSE;

	new(&copy) D3D12PipelineState(device, PSODesc);


	ID3DBlob* fireVS;
	ID3DBlob* firePS;

	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/FireVS.cso", &fireVS);
	if (FAILED(hr)) throw false;
	hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/FirePS.cso", &firePS);
	if (FAILED(hr)) throw false;

	PSODesc.VS.pShaderBytecode = fireVS->GetBufferPointer();
	PSODesc.VS.BytecodeLength = fireVS->GetBufferSize();
	PSODesc.PS.pShaderBytecode = firePS->GetBufferPointer();
	PSODesc.PS.BytecodeLength = firePS->GetBufferSize();

	PSODesc.InputLayout.NumElements = sizeof(texturedInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	PSODesc.InputLayout.pInputElementDescs = texturedInputLayout;
	PSODesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;

	new(&fire) D3D12PipelineState(device, PSODesc);
}

PipelineStateObjects::~PipelineStateObjects() {}