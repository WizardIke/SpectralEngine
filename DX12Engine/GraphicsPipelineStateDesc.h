#pragma once
#include <d3d12.h>

class GraphicsPipelineStateDesc : public D3D12_GRAPHICS_PIPELINE_STATE_DESC
{
public:
	const wchar_t* vertexShaderFileName;
	const wchar_t* pixelShaderFileName;

	constexpr GraphicsPipelineStateDesc(const wchar_t* vertexShaderFileName1, const wchar_t* pixelShaderFileName1, 
		D3D12_STREAM_OUTPUT_DESC StreamOutput, D3D12_BLEND_DESC BlendState, UINT SampleMask, D3D12_RASTERIZER_DESC RasterizerState, D3D12_DEPTH_STENCIL_DESC DepthStencilState, D3D12_INPUT_LAYOUT_DESC InputLayout,
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue, D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType, UINT NumRenderTargets, const DXGI_FORMAT (&RTVFormats)[8], DXGI_FORMAT DSVFormat, DXGI_SAMPLE_DESC SampleDesc) :
		D3D12_GRAPHICS_PIPELINE_STATE_DESC{ nullptr, {nullptr, 0u}, {nullptr, 0u}, {nullptr, 0u}, {nullptr, 0u}, {nullptr, 0u}, 
	StreamOutput, BlendState, SampleMask, RasterizerState, DepthStencilState, InputLayout, IBStripCutValue, PrimitiveTopologyType, NumRenderTargets, 
	{ RTVFormats[0], RTVFormats[1], RTVFormats[2], RTVFormats[3], RTVFormats[4], RTVFormats[5], RTVFormats[6], RTVFormats[7] },
	DSVFormat, SampleDesc, 0u, {nullptr, 0u}, D3D12_PIPELINE_STATE_FLAG_NONE },
		vertexShaderFileName(vertexShaderFileName1),
		pixelShaderFileName(pixelShaderFileName1)
	{}
};