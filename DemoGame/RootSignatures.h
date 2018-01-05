#pragma once
#include <D3D12RootSignature.h>
struct ID3D12Device;

class RootSignatures
{
public:
	RootSignatures(ID3D12Device* const Device);
	~RootSignatures();


	D3D12RootSignature rootSignature;
};