#pragma once

#include "Sentences/CPUUsageSentence.h"
#include "Sentences/FPSSentence.h"
#include <cstdint>
#include <D3D12Resource.h>
class ThreadResources;
class GlobalResources;

class UserInterface
{
public:
	class StopRequest
	{
		friend class UserInterface;
		StopRequest** stopRequest;
		void(*callback)(StopRequest& stopRequest, void* tr);
	public:
		StopRequest(void(*callback1)(StopRequest& stopRequest, void* tr)) : callback(callback1) {}
	};

	struct BasicMaterialPS
	{
		uint32_t baseColorTexture;
	};

	struct TexturedQuadMaterialVS
	{
		float pos[4u]; //stores position x, position y, width, and height
		float texCoords[4u];
	};
private:
	StopRequest* mStopRequest = nullptr;

	CPUUsageSentence CPUUsageSentence;
	FPSSentence FPSSentence;
	D3D12_GPU_VIRTUAL_ADDRESS bufferGpu;
	constexpr static auto BasicMaterialPsSize = (sizeof(BasicMaterialPS) + (std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~((std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (std::size_t)1u);
	constexpr static auto TexturedQuadMaterialVsSize = (sizeof(TexturedQuadMaterialVS) + (std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~((std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (std::size_t)1u);

	void update1();
	void update2(ThreadResources& threadResources);

	bool displayVirtualFeedbackTexture = false;
	D3D12Resource virtualFeedbackTextureCopy;
	unsigned int descriptorIndex;

	GlobalResources& globalResources;

	UserInterface(GlobalResources& globalResources, float aspectRatio);
public:
	UserInterface(GlobalResources& globalResources);
	~UserInterface();

	void setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress);

	/*
	Must be called from primary thread
	*/
	void start(ThreadResources& threadResources);

	/*
	Must be called from primary thread
	*/
	void stop(StopRequest& stopRequest, ThreadResources& threadResources);

	void setFont(const Font& font) noexcept
	{
		CPUUsageSentence.setFont(font);
		FPSSentence.setFont(font);
	}

	void setDisplayVirtualFeedbackTexture(bool value) { displayVirtualFeedbackTexture = value; }
};