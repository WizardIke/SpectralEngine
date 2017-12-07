#include "RootSignatures.h"
#include <d3d12.h>

RootSignatures::RootSignatures(ID3D12Device* const Device) : rootSignature(nullptr)
{
	try
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS options;
		Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

		D3D12_DESCRIPTOR_RANGE  textureDescriptorTableRanges[1];
		textureDescriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		textureDescriptorTableRanges[0].NumDescriptors = options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1 ? 128u : UINT_MAX;
		textureDescriptorTableRanges[0].BaseShaderRegister = 0;
		textureDescriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
		textureDescriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

		D3D12_ROOT_PARAMETER  rootParameters[5];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //main camera
		rootParameters[0].Descriptor.ShaderRegister = 0u;
		rootParameters[0].Descriptor.RegisterSpace = 0u;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //lights
		rootParameters[1].Descriptor.ShaderRegister = 2u;
		rootParameters[1].Descriptor.RegisterSpace = 0u;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //per object
		rootParameters[2].Descriptor.ShaderRegister = 1u;
		rootParameters[2].Descriptor.RegisterSpace = 0u;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //per object
		rootParameters[3].Descriptor.ShaderRegister = 1u;
		rootParameters[3].Descriptor.RegisterSpace = 0u;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; //textures
		rootParameters[4].DescriptorTable.NumDescriptorRanges = 1u;
		rootParameters[4].DescriptorTable.pDescriptorRanges = &textureDescriptorTableRanges[0];
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC staticSampleDescs[3];

		staticSampleDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticSampleDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampleDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampleDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampleDescs[0].MipLODBias = 0;
		staticSampleDescs[0].MaxAnisotropy = 0;
		staticSampleDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSampleDescs[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampleDescs[0].MinLOD = 0.0f;
		staticSampleDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSampleDescs[0].ShaderRegister = 0;
		staticSampleDescs[0].RegisterSpace = 0;
		staticSampleDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		staticSampleDescs[1].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		staticSampleDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampleDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampleDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampleDescs[1].MipLODBias = 0;
		staticSampleDescs[1].MaxAnisotropy = 0;
		staticSampleDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSampleDescs[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampleDescs[1].MinLOD = 0.0f;
		staticSampleDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
		staticSampleDescs[1].ShaderRegister = 1;
		staticSampleDescs[1].RegisterSpace = 0;
		staticSampleDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		staticSampleDescs[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampleDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampleDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampleDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampleDescs[2].MipLODBias = 0;
		staticSampleDescs[2].MaxAnisotropy = 0;
		staticSampleDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSampleDescs[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		staticSampleDescs[2].MinLOD = 0.0f;
		staticSampleDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
		staticSampleDescs[2].ShaderRegister = 2;
		staticSampleDescs[2].RegisterSpace = 0;
		staticSampleDescs[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.NumStaticSamplers = 3u;
		rootSignatureDesc.pStaticSamplers = staticSampleDescs;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		ID3DBlob* errorBuff;
		ID3DBlob* signature;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
		if (FAILED(hr)) throw false;

		hr = Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		if (FAILED(hr)) throw false;
	}
	catch (...)
	{
		if (rootSignature) rootSignature->Release();
		throw;
	}
}

RootSignatures::~RootSignatures()
{
	if (rootSignature) rootSignature->Release();
}