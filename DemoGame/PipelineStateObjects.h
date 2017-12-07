#pragma once
struct ID3D12PipelineState;
struct ID3D12Device;
class RootSignatures;

class PipelineStateObjects
{
public:
	PipelineStateObjects(ID3D12Device* const Device, RootSignatures& rootSignatures);
	~PipelineStateObjects();

	ID3D12PipelineState* textPSO;
	ID3D12PipelineState* lightPSO1;
	ID3D12PipelineState* pointLightPSO1;
	ID3D12PipelineState* waterPSO1;
	ID3D12PipelineState* glassPSO1;
	ID3D12PipelineState* firePSO1;
};