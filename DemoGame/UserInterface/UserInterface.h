#pragma once

#include "Sentences/CPUUsageSentence.h"
#include "Sentences/FPSSentence.h"
#include "../Shaders/BasicMaterialPS.h"
#include "../Shaders/TexturedQuadMaterialVS.h"
#include <D3D12Resource.h>
class Executor;
class SharedResources;

class UserInterface
{
	CPUUsageSentence CPUUsageSentence;
	FPSSentence FPSSentence;
	D3D12_GPU_VIRTUAL_ADDRESS bufferGpu;
	constexpr static auto BasicMaterialPsSize = (sizeof(BasicMaterialPS) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
	constexpr static auto TexturedQuadMaterialVsSize = (sizeof(TexturedQuadMaterialVS) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);

	void update1(SharedResources& sharedResources);
	void update2(Executor* const executor, SharedResources& sharedResources);

	void restart(Executor* const executor);
	bool displayVirtualFeedbackTexture = false;
	D3D12Resource virtualFeedbackTextureCopy;
public:
	UserInterface(SharedResources& sharedResources, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress);
	~UserInterface() {}

	void start(Executor* const executor, SharedResources& sharedResources);

	void setDisplayVirtualFeedbackTexture(bool value) { displayVirtualFeedbackTexture = value; }
};