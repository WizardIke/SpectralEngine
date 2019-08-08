#include "DynamicTextRenderer.h"
#include "HresultException.h"
#include "makeArray.h"

DynamicTextRenderer::DynamicTextRenderer(const unsigned int maxLength, ID3D12Device* const device, std::wstring text, const DirectX::XMFLOAT2 position, const DirectX::XMFLOAT2 size, const DirectX::XMFLOAT4 color) :
	text(std::move(text)),
	size(size),
	maxLength(maxLength),
	color(color),
	position(position),
	textVertexBuffer(makeArray<frameBufferCount>([device, maxLength]()
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

	return D3D12Resource(device, heapProperties, D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ);
}))
{
	HRESULT hr;
#ifndef NDEBUG
	for (auto i = 0u; i != frameBufferCount; ++i)
	{
		wchar_t name[45] = L"Text Vertex Buffer Upload Resource ";
		wchar_t number[5];
		swprintf_s(number, L"%d", i);
		wcscat_s(name, number);
		textVertexBuffer[i]->SetName(name);
	}
#endif

	D3D12_RANGE readRange = { 0u, 0u }; 
	for (auto i = 0u; i != frameBufferCount; ++i)
	{
		hr = textVertexBuffer[i]->Map(0, &readRange, reinterpret_cast<void**>(&textVBGPUAddress[i]));

		textVertexBufferView[i].BufferLocation = textVertexBuffer[i]->GetGPUVirtualAddress();
		textVertexBufferView[i].StrideInBytes = sizeof(TextVertex);
		textVertexBufferView[i].SizeInBytes = maxLength * sizeof(TextVertex);
	}
}

void DynamicTextRenderer::setFont(const Font& newFont) noexcept
{
	if (font != nullptr)
	{
		size.x *= font->width();
		size.y *= font->height();
	}
	font = &newFont;
	size.x /= newFont.width();
	size.y /= newFont.height();
}

void DynamicTextRenderer::beforeRender(uint32_t frameIndex)
{
	assert(font != nullptr);
	float x = position.x;
	float y = position.y;

	TextVertex* vert = textVBGPUAddress[frameIndex];

	wchar_t lastChar = L'\0'; // Last character is used for kerning. L'\0' doesn't have a kerning.
	unsigned int currentVertexIndex = 0u;
	for (auto currentChar : text)
	{
		const auto fontChar = font->getChar(currentChar);
		// character not in font char set
		if (fontChar == nullptr) continue;

		// new line
		if (currentChar == L'\n')
		{
			x = position.x;
			y -= font->height() * size.y;
			continue;
		}

		assert(currentVertexIndex < maxLength && "Trying to render more characters than maxLength");

		float kerning = font->getKerning(lastChar, currentChar);

		new(&vert[currentVertexIndex]) TextVertex
		{ 
			{
				x + ((fontChar->xoffset + kerning) * size.x),
				y - (fontChar->yoffset * size.y),
				fontChar->twidth * size.x,
				fontChar->theight * size.y
			},
			{
				fontChar->u,
				fontChar->v,
				fontChar->twidth,
				fontChar->theight
			},
			{
				color.x,
				color.y,
				color.z,
				color.w
			}
		};

		++currentVertexIndex;

		//advance to next char position
		x += (fontChar->xadvance + kerning) * size.x;

		lastChar = currentChar;
	}
}