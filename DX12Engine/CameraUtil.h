#pragma once
#include <d3d12.h>

namespace CameraUtil
{
	D3D12_VIEWPORT getViewPort(unsigned int width, unsigned int height);
	D3D12_RECT getScissorRect(unsigned int width, unsigned int height);

	void bind(ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end, const D3D12_VIEWPORT& viewPort, const D3D12_RECT& scissorRect, const D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpu,
		const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetView, const D3D12_CPU_DESCRIPTOR_HANDLE* depthSencilView);
}