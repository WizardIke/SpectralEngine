#pragma once
#include <d3d12.h>

namespace CameraUtil
{
	D3D12_VIEWPORT getViewPort(unsigned int width, unsigned int height) noexcept;
	D3D12_RECT getScissorRect(unsigned int width, unsigned int height) noexcept;

	void bind(ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end, const D3D12_VIEWPORT& viewPort, const D3D12_RECT& scissorRect, const D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpu,
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView);
}