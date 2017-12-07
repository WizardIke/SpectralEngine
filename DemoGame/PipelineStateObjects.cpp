#include "PipelineStateObjects.h"
#include <HresultException.h>
#include <d3dcompiler.h>
#include <d3d12.h>
#include "RootSignatures.h"

PipelineStateObjects::PipelineStateObjects(ID3D12Device* const Device, RootSignatures& rootSignatures) :
	textPSO(nullptr),
	lightPSO1(nullptr),
	pointLightPSO1(nullptr),
	waterPSO1(nullptr),
	glassPSO1(nullptr),
	firePSO1(nullptr)
{
	try
	{
		ID3DBlob* TextvertexShaderBuffer;
		ID3DBlob* TextpixelShaderBuffer;

		HRESULT hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/TextVS.cso", &TextvertexShaderBuffer);
		if (FAILED(hr)) throw false;
		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/TextPS.cso", &TextpixelShaderBuffer);
		if (FAILED(hr)) throw false;

		D3D12_INPUT_ELEMENT_DESC TextInputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
		
		PSODesc.pRootSignature = rootSignatures.rootSignature;
		PSODesc.VS.pShaderBytecode = TextvertexShaderBuffer->GetBufferPointer();
		PSODesc.VS.BytecodeLength = TextvertexShaderBuffer->GetBufferSize();
		PSODesc.PS.pShaderBytecode = TextpixelShaderBuffer->GetBufferPointer();
		PSODesc.PS.BytecodeLength = TextpixelShaderBuffer->GetBufferSize();
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
		PSODesc.InputLayout.NumElements = sizeof(TextInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		PSODesc.InputLayout.pInputElementDescs = TextInputLayout;
		PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSODesc.NumRenderTargets = 1;
		PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_UINT? DXGI_FORMAT_R16G16B16A16_FLOAT?
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

		hr = Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&textPSO));
		if (FAILED(hr)) throw false;


		ID3DBlob* vLightShaderBuffer;
		ID3DBlob* pLightShaderBuffer;

		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/vlight.cso", &vLightShaderBuffer);
		if (FAILED(hr)) throw false;
		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/AmbientAndDirectionalLightPS.cso", &pLightShaderBuffer);
		if (FAILED(hr)) throw false;

		D3D12_INPUT_ELEMENT_DESC LightInputLayout[] =
		{
			{ "POSITION", 0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
			{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
			{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
		};

		PSODesc.VS.pShaderBytecode = vLightShaderBuffer->GetBufferPointer();
		PSODesc.VS.BytecodeLength = vLightShaderBuffer->GetBufferSize();
		PSODesc.PS.pShaderBytecode = pLightShaderBuffer->GetBufferPointer();
		PSODesc.PS.BytecodeLength = pLightShaderBuffer->GetBufferSize();
		PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		PSODesc.InputLayout.NumElements = sizeof(LightInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		PSODesc.InputLayout.pInputElementDescs = LightInputLayout;
		
		hr = Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&lightPSO1));
		if (FAILED(hr)) throw false;


		ID3DBlob* vPointLightShaderBuffer;
		ID3DBlob* pPointLightShaderBuffer;

		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/PointLightVS.cso", &vPointLightShaderBuffer);
		if (FAILED(hr)) throw false;
		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/PointLightPS.cso", &pPointLightShaderBuffer);
		if (FAILED(hr)) throw false;

		PSODesc.VS.pShaderBytecode = vPointLightShaderBuffer->GetBufferPointer();
		PSODesc.VS.BytecodeLength = vPointLightShaderBuffer->GetBufferSize();
		PSODesc.PS.pShaderBytecode = pPointLightShaderBuffer->GetBufferPointer();
		PSODesc.PS.BytecodeLength = pPointLightShaderBuffer->GetBufferSize();

		hr = Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&pointLightPSO1));
		if (FAILED(hr)) throw false;


		ID3DBlob* vWaterShaderBuffer;
		ID3DBlob* pWaterShaderBuffer;

		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/vwater.cso", &vWaterShaderBuffer);
		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/pwater.cso", &pWaterShaderBuffer);

		D3D12_INPUT_ELEMENT_DESC WaterInputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		PSODesc.InputLayout.NumElements = sizeof(WaterInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		PSODesc.InputLayout.pInputElementDescs = WaterInputLayout;
		PSODesc.VS.pShaderBytecode = vWaterShaderBuffer->GetBufferPointer();
		PSODesc.VS.BytecodeLength = vWaterShaderBuffer->GetBufferSize();
		PSODesc.PS.pShaderBytecode = pWaterShaderBuffer->GetBufferPointer();
		PSODesc.PS.BytecodeLength = pWaterShaderBuffer->GetBufferSize();

		hr = Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&waterPSO1));
		if (FAILED(hr)) throw false;


		ID3DBlob* vGlassShaderBuffer;
		ID3DBlob* pGlassShaderBuffer;

		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/vglass.cso", &vGlassShaderBuffer);
		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/pglass.cso", &pGlassShaderBuffer);

		PSODesc.VS.pShaderBytecode = vGlassShaderBuffer->GetBufferPointer();
		PSODesc.VS.BytecodeLength = vGlassShaderBuffer->GetBufferSize();
		PSODesc.PS.pShaderBytecode = pGlassShaderBuffer->GetBufferPointer();
		PSODesc.PS.BytecodeLength = pGlassShaderBuffer->GetBufferSize();

		hr = Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&glassPSO1));
		if (FAILED(hr)) throw false;


		ID3DBlob* vFireShaderBuffer;
		ID3DBlob* pFireShaderBuffer;

		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/vfire.cso", &vFireShaderBuffer);
		hr = D3DReadFileToBlob(L"../DemoGame/CompiledShaders/pfire.cso", &pFireShaderBuffer);

		PSODesc.VS.pShaderBytecode = vFireShaderBuffer->GetBufferPointer();
		PSODesc.VS.BytecodeLength = vFireShaderBuffer->GetBufferSize();
		PSODesc.PS.pShaderBytecode = pFireShaderBuffer->GetBufferPointer();
		PSODesc.PS.BytecodeLength = pFireShaderBuffer->GetBufferSize();

		PSODesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;

		hr = Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&firePSO1));
		if (FAILED(hr)) throw false;
	}
	catch (...)
	{
		this->~PipelineStateObjects();
		throw;
	}
}

PipelineStateObjects::~PipelineStateObjects()
{
	if (textPSO) textPSO->Release();
	if (lightPSO1) lightPSO1->Release();
	if (pointLightPSO1) pointLightPSO1->Release();
	if (waterPSO1) waterPSO1->Release();
	if (glassPSO1) glassPSO1->Release();
	if (firePSO1) firePSO1->Release();
}