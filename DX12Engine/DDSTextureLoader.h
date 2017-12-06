/*****************************************************************
* Copyright (c) 2015 Leif Erkenbrach
* Distributed under the terms of the MIT License.
* (See accompanying file LICENSE or copy at
* http://opensource.org/licenses/MIT)
*****************************************************************/

/**
*	Light-weight runtime DDS file loader
*
*	Port of DDSTextureLoader to support the Direct3D 12 runtime by Leif Erkenbrach
*
*	Does not support automatic mip-mapping at the moment because Direct3D 12 does not support it.
*	You will need to generate the mipmaps in an external tool for now.
*
*	Note: Loaded texture resources are currently stored as commited resources with the heap type D3D12_HEAP_TYPE_UPLOAD.
*			D3D12_HEAP_TYPE_UPLOAD is not recommended for use on resources commonly sampled by the GPU.
*			In some artificial tests it took about 30x as long to sample from an upload heap instead of a default heap.
*			Though the times will differ based on the hardware and driver.
*			At the moment it is left up to the user to initialize a heap with the type D3D12_HEAP_TYPE_DEFAULT and copy the loaded texture to that heap.
*
*	Original D3D11 version: http://go.microsoft.com/fwlink/?LinkId=248926
*/

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

	// Standard version
	HRESULT CreateDDSTextureFromMemory(_In_ ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* d3dContext,
		_In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
		_In_ size_t ddsDataSize,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_In_ size_t maxsize = 0,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	HRESULT CreateDDSTextureFromMemory(_In_ ID3D12Device* d3dDevice,
		_In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
		_In_ size_t ddsDataSize,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_In_ size_t maxsize = 0,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);


	HRESULT CreateDDSTextureFromFile(_In_ ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* d3dContext,
		_In_z_ const wchar_t* szFileName,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_In_ size_t maxsize = 0,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	HRESULT CreateDDSTextureFromFile(_In_ ID3D12Device* d3dDevice,
		_In_z_ const wchar_t* szFileName,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_In_ size_t maxsize = 0,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	HRESULT CreateDDSTextureFromFile(_In_ ID3D12Device* d3dDevice, //I added this
		_In_z_ const wchar_t* szFileName,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Outptr_opt_ D3D12_RESOURCE_DESC* textureDesc,
		_In_ size_t maxsize = 0,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	// Extended version
	HRESULT CreateDDSTextureFromMemoryEx(_In_ ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* d3dContext,
		_In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
		_In_ size_t ddsDataSize,
		_In_ size_t maxsize,
		_In_ bool forceSRGB,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	HRESULT CreateDDSTextureFromMemoryEx(_In_ ID3D12Device* d3dDevice,
		_In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
		_In_ size_t ddsDataSize,
		_In_ size_t maxsize,
		_In_ bool forceSRGB,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	HRESULT CreateDDSTextureFromFileEx(_In_ ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* d3dContext,
		_In_z_ const wchar_t* szFileName,
		_In_ size_t maxsize,
		_In_ bool forceSRGB,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	//I added this
	HRESULT CreateDDSTextureFromFileEx(_In_ ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* d3dContext,
		_In_z_ const wchar_t* szFileName,
		_In_ size_t maxsize,
		_In_ bool forceSRGB,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Outptr_opt_ D3D12_RESOURCE_DESC* textureDesc,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);

	HRESULT CreateDDSTextureFromFileEx(_In_ ID3D12Device* d3dDevice,
		_In_z_ const wchar_t* szFileName,
		_In_ size_t maxsize,
		_In_ bool forceSRGB,
		_Outptr_ ID3D12Resource** texture,
		_Outptr_opt_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
		_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
	);
}