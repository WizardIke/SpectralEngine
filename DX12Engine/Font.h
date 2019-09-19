#pragma once

#include <cstddef> //std::size_t
#include <cstdint> //uint64_t, uint32_t
#include <d3d12.h> //D3D12_GPU_VIRTUAL_ADDRESS
#include "TextureManager.h"
#include "AsynchronousFileManager.h"
#include "ResourceLocation.h"

class Font
{
private:
	template<class ThreadResources>
	static void fontFileLoaded(AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager&, void* tr, const unsigned char* data)
	{
		LoadRequest& loadRequest = static_cast<LoadRequest&>(static_cast<FontFileLoadRequest&>(request));
		fontFileLoadedHelper(loadRequest, data);
		TextureManager& textureManager = *loadRequest.textureManager;
		textureManager.load(static_cast<TextureManager::TextureStreamingRequest&>(loadRequest), *static_cast<ThreadResources*>(tr));
	}

	static void textureLoaded(TextureManager::TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor);

	struct FontFileLoadRequest : public AsynchronousFileManager::ReadRequest 
	{
		using AsynchronousFileManager::ReadRequest::ReadRequest;
	};
public:
	struct Kerning
	{
		uint64_t firstIdAndSecoundId;
		float amount; // the amount to add to second characters x in texture space
	};

	class Character
	{
	public:
		uint32_t id;
		float u; // u texture coordinate
		float v; // v texture coordinate
		float twidth; // width of character in texture space
		float theight; // height of character in texture space
		float xoffset; // offset from current cursor pos to left side of character in texture space
		float yoffset; // offset from top of line to top of character in texture space
		float xadvance; // how far to move to right for next character in texture space
	};

	class LoadRequest : FontFileLoadRequest, TextureManager::TextureStreamingRequest
	{
		friend class Font;
		Font* font;
		void(*fontLoadedCallback)(LoadRequest& request, void* tr);
		TextureManager* textureManager;
	public:
		LoadRequest(ResourceLocation resourceLocation, void(*fontLoadedCallback1)(LoadRequest& request, void* tr)) :
			fontLoadedCallback(fontLoadedCallback1)
		{
			auto& fileRequest = *static_cast<FontFileLoadRequest*>(this);
			fileRequest.start = resourceLocation.start;
			fileRequest.end = resourceLocation.end;
		}
	};

	class UnloadRequest : FontFileLoadRequest, TextureManager::Message
	{
		friend class Font;
		void(*fontUnloadedCallback)(UnloadRequest& request, void* tr);
		AsynchronousFileManager* asynchronousFileManager;
	public:
		UnloadRequest(ResourceLocation resourceLocation, void(*fontUnloadedCallback1)(UnloadRequest& request, void* tr)) :
			fontUnloadedCallback{ fontUnloadedCallback1 }
		{
			auto& fileRequest = *static_cast<FontFileLoadRequest*>(this);
			fileRequest.start = resourceLocation.start;
			fileRequest.end = resourceLocation.end;
		}
	};
private:
	static void fontFileLoadedHelper(LoadRequest& loadRequest, const unsigned char* data);

	struct PSPerObjectConstantBuffer
	{
		uint32_t diffuseTexture;
	};
	constexpr static std::size_t psPerObjectConstantBufferSize = (sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;
	PSPerObjectConstantBuffer* psPerObjectCBVCpuAddress;
	std::size_t dataSize;
	const unsigned char* data;
	float mWidth; //The width of the square that will fit a charactor in texture space
	float mHeight; //The height of the square that will fit a charactor in texture space
	
	unsigned long numCharacters;
	const Character* charList; //sorted array was faster than unordered_map
	unsigned long numKernings;
	const Kerning* kerningsList; //sorted array was faster than unordered_map
public:
	// this will return the amount of kerning we need to use for two characters
	float getKerning(wchar_t first, wchar_t second) const noexcept;

	// this will return a FontChar given a wide character
	const Character* getChar(wchar_t c) const noexcept;

	float width() const noexcept { return mWidth; }
	float height() const noexcept { return mHeight; }
	D3D12_GPU_VIRTUAL_ADDRESS getConstantBuffer() const noexcept { return psPerObjectCBVGpuAddress; }

	Font() {}
	~Font() = default;

	//Must be called before load
	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress);

	template<class ThreadResources>
	void load(LoadRequest& loadRequest, AsynchronousFileManager& asynchronousFileManager, TextureManager& textureManager, ThreadResources&)
	{
		loadRequest.textureManager = &textureManager;
		loadRequest.font = this;
		auto& fileRequest = static_cast<FontFileLoadRequest&>(loadRequest);
		fileRequest.fileLoadedCallback = fontFileLoaded<ThreadResources>;
		asynchronousFileManager.read(fileRequest);
	}

	template<class ThreadResources>
	void destruct(UnloadRequest& unloadRequest, AsynchronousFileManager& asynchronousFileManager, TextureManager& textureManager, ThreadResources& threadResources)
	{
		unloadRequest.asynchronousFileManager = &asynchronousFileManager;

		auto& textureRequest = static_cast<TextureManager::Message&>(unloadRequest);
		textureRequest.start = *reinterpret_cast<const uint64_t*>(data);
		textureRequest.end = *reinterpret_cast<const uint64_t*>(data + sizeof(uint64_t));
		textureRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void*)
		{
			auto& unloadRequest = static_cast<UnloadRequest&>(static_cast<TextureManager::Message&>(request));
			AsynchronousFileManager& asynchronousFileManager = *unloadRequest.asynchronousFileManager;
			auto& fileRequest = static_cast<FontFileLoadRequest&>(unloadRequest);
			asynchronousFileManager.discard(fileRequest);
		};

		auto& fileRequest = static_cast<FontFileLoadRequest&>(unloadRequest);
		fileRequest.deleteReadRequest = [](AsynchronousFileManager::ReadRequest& request, void* tr)
		{
			auto& unloadRequest = static_cast<UnloadRequest&>(static_cast<FontFileLoadRequest&>(request));
			unloadRequest.fontUnloadedCallback(unloadRequest, tr);
		};

		textureManager.unload(textureRequest, threadResources);
	}
};