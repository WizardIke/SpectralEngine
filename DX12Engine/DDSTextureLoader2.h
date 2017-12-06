#pragma once

#include <d3d12.h>

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0601

#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#pragma warning(pop)

#if defined(_MSC_VER) && (_MSC_VER<1610) && !defined(_In_reads_)
#define _In_reads_(exp)
#define _Out_writes_(exp)
#define _In_reads_bytes_(exp)
#define _In_reads_opt_(exp)
#define _Outptr_opt_
#endif

#ifndef _Use_decl_annotations_
#define _Use_decl_annotations_
#endif

namespace DirectX
{
	enum DDS_ALPHA_MODE
	{
		DDS_ALPHA_MODE_UNKNOWN = 0,
		DDS_ALPHA_MODE_STRAIGHT = 1,
		DDS_ALPHA_MODE_PREMULTIPLIED = 2,
		DDS_ALPHA_MODE_OPAQUE = 3,
		DDS_ALPHA_MODE_CUSTOM = 4,
	};

	HRESULT CreateDDSTextureFromFile(_In_ ID3D12Device* d3dDevice,
		_In_z_ const wchar_t* szFileName,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Outptr_opt_ D3D12_RESOURCE_DESC* textureDesc,
		_In_ UINT& arraySize,
		_Out_writes_to_ptr_(p) D3D12_SUBRESOURCE_DATA** initData,
		_In_ size_t maxsize = 0,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);


	HRESULT CreateDDSTextureFromFileEx(_In_ ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* d3dContext,
		_In_z_ const wchar_t* szFileName,
		_In_ size_t maxsize,
		_In_ bool forceSRGB,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Outptr_opt_ D3D12_RESOURCE_DESC* textureDesc,
		_In_ UINT& arraySize,
		_Out_writes_to_ptr_(p) D3D12_SUBRESOURCE_DATA** initData,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);
}