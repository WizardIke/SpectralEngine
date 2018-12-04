#include "BaseSentence.h"
#include "HresultException.h"

constexpr inline float setPositionX(const float pos)
{
	return (pos * 2.0f) - 1.0f;
}

constexpr inline float setPositionY(const float pos)
{
	return ((1.0f - pos) * 2.0f) - 1.0f;
}


BaseSentence::BaseSentence(const unsigned int maxLength, ID3D12Device* const Device, Font* const Font, const wchar_t* Text, const DirectX::XMFLOAT2 Position, const DirectX::XMFLOAT2 Size, const DirectX::XMFLOAT4 color) :
	font(Font),
	text(Text),
	size(Size),
	maxLength(maxLength),
	color(color),
	position(setPositionX(Position.x), setPositionY(Position.y)), 
	textVertexBuffer([Device, maxLength](std::size_t, D3D12Resource& element)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 0u;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.VisibleNodeMask = 0u;

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	resourceDesc.Height = 1u;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	resourceDesc.Width = maxLength * sizeof(TextVertex);

	new(&element) D3D12Resource(Device, heapProperties, D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ);
})
{
	HRESULT hr;
#ifndef NDEBUG
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		wchar_t name[45] = L"Text Vertex Buffer Upload Resource ";
		wchar_t number[5];
		swprintf_s(number, L"%d", i);
		wcscat_s(name, number);
		textVertexBuffer[i]->SetName(name);
	}
#endif

	D3D12_RANGE readRange = { 0u, 0u }; 
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		hr = textVertexBuffer[i]->Map(0, &readRange, reinterpret_cast<void**>(&textVBGPUAddress[i]));

		textVertexBufferView[i].BufferLocation = textVertexBuffer[i]->GetGPUVirtualAddress();
		textVertexBufferView[i].StrideInBytes = sizeof(TextVertex);
		textVertexBufferView[i].SizeInBytes = maxLength * sizeof(TextVertex);
	}
}

BaseSentence::~BaseSentence() {}

void BaseSentence::update(uint32_t frameIndex)
{
	unsigned int numCharacters = 0u;

	float horrizontalPadding = font->leftpadding + font->rightpadding;
	float verticalPadding = font->toppadding + font->bottompadding;

	float x = position.x;
	float y = position.y;

	TextVertex* vert = textVBGPUAddress[frameIndex];

	wchar_t lastChar = ' '; // no last character to start with

	for (auto i = 0u; i < text.size(); ++i)
	{
		auto currentChar = text[i];

		auto fontChar = font->getChar(currentChar);

		// character not in font char set
		if (!fontChar) continue;

		// end of string
		if (currentChar == L'\0') break;

		// new line
		if (currentChar == L'\n')
		{
			x = position.x;
			y -= (font->lineHeight + verticalPadding) * size.y;
			continue;
		}

		if (numCharacters >= maxLength)
		{
			break;
		}

		float kerning = 0.0f;
		if (i != 0u) kerning = font->getKerning(lastChar, currentChar);

		vert[numCharacters] = TextVertex(color.x,
			color.y,
			color.z,
			color.w,
			fontChar->u,
			fontChar->v,
			fontChar->twidth,
			fontChar->theight,
			x + ((fontChar->xoffset + kerning) * size.x),
			y - (fontChar->yoffset * size.y),
			fontChar->width * size.x,
			fontChar->height * size.y);

		++numCharacters;

		// remove horrizontal padding and advance to next char position
		x += (fontChar->xadvance - horrizontalPadding) * size.x;

		lastChar = currentChar;
	}
}

void BaseSentence::setPosition(const DirectX::XMFLOAT2 pos)
{
	position.x = setPositionX(pos.x);
	position.y = setPositionY(pos.y);
}