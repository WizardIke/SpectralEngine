#pragma once

#include <string>
#include "FontChar.h"
#include <memory>
#include <d3d12.h>

class BaseExecutor;

struct Font
{
	struct FontKerning
	{
		uint64_t firstIdAndSecoundId; // the first character
		float amount; // the amount to add/subtract to second characters x
	};
	struct PSPerObjectConstantBuffer
	{
		uint32_t diffuseTexture;
	};

	constexpr static std::size_t psPerObjectConstantBufferSize = (sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

	std::wstring name; // name of the font
	int size; // size of font, lineheight and baseheight will be based on this as if this is a single unit (1.0)
	float lineHeight; // will be normalized
	float baseHeight; // will be normalized
	int textureWidth; // width of the font texture
	int textureHeight; // height of the font texture
	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;
	unsigned int numCharacters; // number of characters in the font
	std::unique_ptr<FontChar[]> charList;
	unsigned int numKernings; // the number of kernings
	std::unique_ptr<FontKerning[]> kerningsList; //sorted array was faster than unordered_map

	float leftpadding;					   // these are how much the character is padded in the texture. We
	float toppadding;					   // add padding to give sampling a little space so it does not accidentally
	float rightpadding;					   // padd the surrounding characters. We will need to subtract these paddings
	float bottompadding;				   // from the actual spacing between characters to remove the gaps you would otherwise see


	// this will return the amount of kerning we need to use for two characters
	float getKerning(wchar_t first, wchar_t second);

	// this will return a FontChar given a wide character
	FontChar* getChar(wchar_t c);

	Font() {}

	template<class Executor>
	Font(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, const wchar_t* const filename, const wchar_t* const textureFile, Executor* const executor, 
		void* requester, void(*callback)(void* requester, BaseExecutor* const executor, unsigned int textureID))
	{
		executor->sharedResources->textureManager.loadTexture(executor, textureFile, { requester, callback });
		auto windowWidth = executor->sharedResources->window.width();
		auto windowHeight = executor->sharedResources->window.height();
		create(constantBufferGpuAddress, constantBufferCpuAddress, filename, textureFile, windowWidth, windowHeight);
	}

	void destruct(BaseExecutor* const executor, const wchar_t* const textureFile);
	~Font() {}

	void setDiffuseTexture(uint32_t diffuseTexture, uint8_t* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<PSPerObjectConstantBuffer*>(cpuStartAddress + (psPerObjectCBVGpuAddress - gpuStartAddress));
		buffer->diffuseTexture = diffuseTexture;
	}
private:
	void create(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, const wchar_t* const filename, const wchar_t* const textureFile,
		unsigned int windowWidth, unsigned int windowHeight);
};