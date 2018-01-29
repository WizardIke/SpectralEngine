#pragma once

#include "frameBufferCount.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include "Font.h"
#include "../Array/Array.h"
#include "D3D12Resource.h"

class BaseSentence
{
	Array<D3D12Resource, frameBufferCount> textVertexBuffer;
	unsigned short maxLength;
	DirectX::XMFLOAT2 position;

	struct TextVertex {
		TextVertex(float r, float g, float b, float a, float u, float v, float tw, float th, float x, float y, float w, float h) : color(r, g, b, a), texCoord(u, v, tw, th), pos(x, y, w, h) {}
		TextVertex(){}
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 texCoord;
		DirectX::XMFLOAT4 color;
	};

	TextVertex* textVBGPUAddress[frameBufferCount];
public:
	BaseSentence(const unsigned int MaxLength, ID3D12Device* const Device, Font* const Font, const wchar_t* Text, const DirectX::XMFLOAT2 Size, const DirectX::XMFLOAT2 Padding, const DirectX::XMFLOAT4 color);
	
	~BaseSentence();

	void setPosition(const DirectX::XMFLOAT2 pos);
	void update(uint32_t FrameIndex);

	Font* font;
	std::wstring text;
	DirectX::XMFLOAT2 size;
	DirectX::XMFLOAT4 color;

	D3D12_VERTEX_BUFFER_VIEW textVertexBufferView[frameBufferCount];
};