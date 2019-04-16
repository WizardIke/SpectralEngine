#pragma once

#include "Sentences/CPUUsageSentence.h"
#include "Sentences/FPSSentence.h"
#include "../Shaders/BasicMaterialPS.h"
#include "../Shaders/TexturedQuadMaterialVS.h"
#include <D3D12Resource.h>
class ThreadResources;
class GlobalResources;

class UserInterface
{
public:
	class StopRequest
	{
	public:
		void(*callback)(StopRequest& stopRequest, void* tr, void* gr);
	};
private:
	std::atomic<StopRequest*> mStopRequest = nullptr;

	CPUUsageSentence CPUUsageSentence;
	FPSSentence FPSSentence;
	D3D12_GPU_VIRTUAL_ADDRESS bufferGpu;
	constexpr static auto BasicMaterialPsSize = (sizeof(BasicMaterialPS) + (std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~((std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (std::size_t)1u);
	constexpr static auto TexturedQuadMaterialVsSize = (sizeof(TexturedQuadMaterialVS) + (std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~((std::size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (std::size_t)1u);

	void update1(GlobalResources& sharedResources);
	void update2(ThreadResources& executor, GlobalResources& sharedResources);

	bool displayVirtualFeedbackTexture = false;
	D3D12Resource virtualFeedbackTextureCopy;
public:
	UserInterface() {}
	UserInterface(GlobalResources& sharedResources, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress);
	~UserInterface() {}

	/*
	Must be called from primary thread
	*/
	void start(ThreadResources& executor, GlobalResources& sharedResources);

	void stop(StopRequest& stopRequest);

	void setDisplayVirtualFeedbackTexture(bool value) { displayVirtualFeedbackTexture = value; }
};