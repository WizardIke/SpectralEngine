#include "CameraUtil.h"

namespace CameraUtil
{
	D3D12_VIEWPORT getViewPort(unsigned int width, unsigned int height) noexcept
	{
		D3D12_VIEWPORT viewPort;
		viewPort.Height = static_cast<FLOAT>(height);
		viewPort.Width = static_cast<FLOAT>(width);
		viewPort.MaxDepth = 1.0f;
		viewPort.MinDepth = 0.0f;
		viewPort.TopLeftX = 0.0f;
		viewPort.TopLeftY = 0.0f;
		return viewPort;
	}

	D3D12_RECT getScissorRect(unsigned int width, unsigned int height) noexcept
	{
		D3D12_RECT scissorRect;
		scissorRect.top = 0;
		scissorRect.left = 0;
		scissorRect.bottom = height;
		scissorRect.right = width;
		return scissorRect;
	}

	void bind(ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end, const D3D12_VIEWPORT& viewPort, const D3D12_RECT& scissorRect, const D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpu,
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView)
	{
		for (auto start = first; start != end; ++start)
		{
			auto commandList = *start;
			commandList->RSSetViewports(1u, &viewPort);
			commandList->RSSetScissorRects(1u, &scissorRect);
			commandList->SetGraphicsRootConstantBufferView(0u, constantBufferGpu);
			commandList->OMSetRenderTargets(1u, &renderTargetView, TRUE, &depthSencilView);
		}
	}
}