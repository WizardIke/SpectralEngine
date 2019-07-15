#pragma once

#include <string>
#include "FontChar.h"
#include <memory>
#include <d3d12.h>
#include "TextureManager.h"
#include "Window.h"
class GraphicsEngine;

struct Font
{
	struct FontKerning
	{
		uint64_t firstIdAndSecoundId;
		float amount; // the amount to add to second characters x
	};
	struct PSPerObjectConstantBuffer
	{
		uint32_t diffuseTexture;
	};

	constexpr static std::size_t psPerObjectConstantBufferSize = (sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

	float lineHeight; //how much a newline charactors moves down by.
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;
	unsigned int numCharacters;
	std::unique_ptr<FontChar[]> charList;
	unsigned int numKernings;
	std::unique_ptr<FontKerning[]> kerningsList; //sorted array was faster than unordered_map


	// this will return the amount of kerning we need to use for two characters
	float getKerning(wchar_t first, wchar_t second) const;

	// this will return a FontChar given a wide character
	const FontChar* getChar(wchar_t c) const;

	template<class ThreadResources>
	Font(const wchar_t* const filename, ThreadResources& threadResources, TextureManager& textureManager,
		float aspectRatio, TextureManager::TextureStreamingRequest& textureRequest)
	{
		textureManager.load(textureRequest, threadResources);
		create(filename, aspectRatio);
	}

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress);

	template<class ThreadResources>
	void destruct(ThreadResources& threadResources, TextureManager& textureManager, const wchar_t* const textureFile)
	{
		auto textureUnloader = new TextureManager::Message(textureFile, [](AsynchronousFileManager::ReadRequest& request, void*)
		{
			delete static_cast<TextureManager::Message*>(&request);
		});
		textureManager.unload(*textureUnloader, threadResources);
	}

	~Font() = default;

	void setDiffuseTexture(uint32_t diffuseTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<PSPerObjectConstantBuffer*>(cpuStartAddress + (psPerObjectCBVGpuAddress - gpuStartAddress));
		buffer->diffuseTexture = diffuseTexture;
	}
private:
	void create(const wchar_t* const filename, float aspectRatio);
};