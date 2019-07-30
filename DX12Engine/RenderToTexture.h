#pragma once
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include "GraphicsEngine.h"

class RenderToTexture
{
	D3D12Resource mResource;
	D3D12DescriptorHeap mCpuDescriptorHeap;
	unsigned int mDescriptorIndex;
public:
	RenderToTexture(unsigned int width, unsigned int height, GraphicsEngine& graphicsEngine) :
		mResource(graphicsEngine.graphicsDevice, []()
			{
				D3D12_HEAP_PROPERTIES properties;
				properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
				properties.VisibleNodeMask = 0u;
				properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				properties.CreationNodeMask = 0u;
				properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
				return properties;
			}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			[width, height]()
			{
				D3D12_RESOURCE_DESC resourceDesc;
				resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				resourceDesc.Width = width;
				resourceDesc.Height = height;
				resourceDesc.DepthOrArraySize = 1u;
				resourceDesc.MipLevels = 1u;
				resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				resourceDesc.SampleDesc.Count = 1u;
				resourceDesc.SampleDesc.Quality = 0u;
				resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				return resourceDesc;
			}(),
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		mCpuDescriptorHeap(graphicsEngine.graphicsDevice, []()
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc;
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				desc.NumDescriptors = 1;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				desc.NodeMask = 0;
				return desc;
			}()),
		mDescriptorIndex(graphicsEngine.descriptorAllocator.allocate())
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1u;
		srvDesc.Texture2D.MostDetailedMip = 0u;
		srvDesc.Texture2D.PlaneSlice = 0u;
		srvDesc.Texture2D.ResourceMinLODClamp = 0u;
		graphicsEngine.graphicsDevice->CreateShaderResourceView(mResource, &srvDesc, graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
			graphicsEngine.cbvAndSrvAndUavDescriptorSize * mDescriptorIndex);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0u;
		rtvDesc.Texture2D.PlaneSlice = 0u;
		graphicsEngine.graphicsDevice->CreateRenderTargetView(mResource, &rtvDesc, mCpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

#ifndef NDEBUG
		mResource->SetName(L"Render to texture resource");
		mCpuDescriptorHeap->SetName(L"Render to texture descriptor heap");
#endif
	}

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptorHandle()
	{
		return mCpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	unsigned int shaderResourceViewDescriptorIndex() const
	{
		return mDescriptorIndex;
	}

	ID3D12Resource& resource()
	{
		return *mResource;
	}

	void destruct(GraphicsEngine& graphicsEngine)
	{
		graphicsEngine.descriptorAllocator.deallocate(mDescriptorIndex);
	}

	void resize(unsigned int width, unsigned int height, GraphicsEngine& graphicsEngine)
	{
		mResource = D3D12Resource(graphicsEngine.graphicsDevice, []()
			{
				D3D12_HEAP_PROPERTIES properties;
				properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
				properties.VisibleNodeMask = 0u;
				properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				properties.CreationNodeMask = 0u;
				properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
				return properties;
			}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
				[width, height]()
			{
				D3D12_RESOURCE_DESC resourceDesc;
				resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				resourceDesc.Width = width;
				resourceDesc.Height = height;
				resourceDesc.DepthOrArraySize = 1u;
				resourceDesc.MipLevels = 1u;
				resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				resourceDesc.SampleDesc.Count = 1u;
				resourceDesc.SampleDesc.Quality = 0u;
				resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				return resourceDesc;
			}(),
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1u;
		srvDesc.Texture2D.MostDetailedMip = 0u;
		srvDesc.Texture2D.PlaneSlice = 0u;
		srvDesc.Texture2D.ResourceMinLODClamp = 0u;
		graphicsEngine.graphicsDevice->CreateShaderResourceView(mResource, &srvDesc, graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
			graphicsEngine.cbvAndSrvAndUavDescriptorSize * mDescriptorIndex);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0u;
		rtvDesc.Texture2D.PlaneSlice = 0u;
		graphicsEngine.graphicsDevice->CreateRenderTargetView(mResource, &rtvDesc, mCpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}
};