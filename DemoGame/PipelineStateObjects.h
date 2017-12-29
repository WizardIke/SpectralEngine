#pragma once
#include <D3D12PipelineState.h>
struct ID3D12Device;
class RootSignatures;

class PipelineStateObjects
{
public:
	PipelineStateObjects(ID3D12Device* const Device, RootSignatures& rootSignatures);
	~PipelineStateObjects();

	D3D12PipelineState text;
	D3D12PipelineState directionalLight;
	D3D12PipelineState pointLight;
	D3D12PipelineState waterWithReflectionTexture;
	D3D12PipelineState waterNoReflectionTexture;
	D3D12PipelineState glass;
	D3D12PipelineState basic;
	D3D12PipelineState fire;
	D3D12PipelineState copy;
};