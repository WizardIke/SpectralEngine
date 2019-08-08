#pragma once

#include "frameBufferCount.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include "Font.h"
#include <array>
#include "D3D12Resource.h"

class DynamicTextRenderer
{
	struct TextVertex {
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 texCoord;
		DirectX::XMFLOAT4 color;
	};

	std::array<D3D12Resource, frameBufferCount> textVertexBuffer;
	unsigned int maxLength;
	TextVertex* textVBGPUAddress[frameBufferCount];
	const Font* font = nullptr;
	DirectX::XMFLOAT2 position;
	DirectX::XMFLOAT2 size;
public:
	DynamicTextRenderer(unsigned int maxLength, ID3D12Device* Device, std::wstring text, DirectX::XMFLOAT2 Position, DirectX::XMFLOAT2 Size, DirectX::XMFLOAT4 color);
	
	~DynamicTextRenderer() = default;

	void beforeRender(uint32_t FrameIndex);

	void setFont(const Font& font) noexcept;

	std::wstring text;
	DirectX::XMFLOAT4 color;

	D3D12_VERTEX_BUFFER_VIEW textVertexBufferView[frameBufferCount];
};