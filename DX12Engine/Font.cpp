#include "Font.h"
#include "GraphicsEngine.h"
#include "TextureManager.h"
#include <fstream>

float Font::getKerning(const wchar_t first, const wchar_t second) const noexcept
{
	const uint64_t searchId = (static_cast<uint64_t>(first) << 32u) | static_cast<uint64_t>(second);
	auto i = std::lower_bound(kerningsList, kerningsList + numKernings, searchId, [](const Kerning& lhs, uint64_t rhs) {return lhs.firstIdAndSecoundId < rhs; });
	if (i->firstIdAndSecoundId == searchId)
	{
		return i->amount;
	}
	return 0.0f;
}

const Font::Character* Font::getChar(const wchar_t c) const noexcept
{
	auto i = std::lower_bound(charList, charList + numCharacters, c, [](const Character& lhs, wchar_t rhs) {return lhs.id < rhs; });
	if (i->id == c)
	{
		return i;
	}
	return nullptr;
}

void Font::setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress)
{
	psPerObjectCBVCpuAddress = reinterpret_cast<PSPerObjectConstantBuffer*>(constantBufferCpuAddress);
	constantBufferCpuAddress += psPerObjectConstantBufferSize;
	psPerObjectCBVGpuAddress = constantBufferGpuAddress;
	constantBufferGpuAddress += psPerObjectConstantBufferSize;
}

static constexpr std::size_t alignedLength(std::size_t length, std::size_t alignment) noexcept
{
	return (length + (alignment - 1u)) & ~(alignment - 1u);
}

void Font::fontFileLoadedHelper(LoadRequest& loadRequest, const unsigned char* data)
{
	Font& font = *loadRequest.font;
	font.dataSize = static_cast<std::size_t>(static_cast<FontFileLoadRequest&>(loadRequest).end - static_cast<FontFileLoadRequest&>(loadRequest).start);
	font.data = data;
	std::size_t currentDataPosition = 0u;
	unsigned long long textureStart = *reinterpret_cast<const uint64_t*>(data + currentDataPosition);
	currentDataPosition += sizeof(uint64_t);
	unsigned long long textureEnd = *reinterpret_cast<const uint64_t*>(data + currentDataPosition);
	currentDataPosition += sizeof(uint64_t);
	font.mWidth = *reinterpret_cast<const float*>(data + currentDataPosition);
	currentDataPosition += sizeof(float);
	font.mHeight = *reinterpret_cast<const float*>(data + currentDataPosition);
	currentDataPosition += sizeof(float);
	font.numCharacters = *reinterpret_cast<const uint32_t*>(data + currentDataPosition);
	currentDataPosition += sizeof(uint32_t);
	font.charList = reinterpret_cast<const Character*>(data + currentDataPosition);
	currentDataPosition += sizeof(Character) * font.numCharacters;
	font.numKernings = *reinterpret_cast<const uint32_t*>(data + currentDataPosition);
	currentDataPosition += sizeof(uint32_t);
	currentDataPosition = alignedLength(currentDataPosition, alignof(Kerning));
	font.kerningsList = reinterpret_cast<const Kerning*>(data + currentDataPosition);

	TextureManager::TextureStreamingRequest& textureRequest = static_cast<TextureManager::TextureStreamingRequest&>(loadRequest);
	textureRequest.textureLoaded = textureLoaded;
	textureRequest.resourceLocation.start = textureStart;
	textureRequest.resourceLocation.end = textureEnd;
}

void Font::textureLoaded(TextureManager::TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor)
{
	LoadRequest& loadRequest = static_cast<LoadRequest&>(request);
	auto& font = *loadRequest.font;
	font.psPerObjectCBVCpuAddress->diffuseTexture = textureDescriptor;
	loadRequest.fontLoadedCallback(loadRequest, tr);
}