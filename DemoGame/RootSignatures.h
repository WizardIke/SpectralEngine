#pragma once
struct ID3D12RootSignature;
struct ID3D12Device;

class RootSignatures
{
public:
	RootSignatures(ID3D12Device* const Device);
	~RootSignatures();


	ID3D12RootSignature* rootSignature;
};