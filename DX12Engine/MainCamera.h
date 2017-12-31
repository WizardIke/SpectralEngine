#pragma once
#include "Camera.h"
#include "D3D12DescriptorHeap.h"

class MainCamera : public Camera
{
	D3D12DescriptorHeap renderTargetViewDescriptorHeap;
	uint32_t backBufferTextures[frameBufferCount];
public:
	MainCamera() {}
	MainCamera(SharedResources* sharedResources, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
		uint8_t*& constantBufferCpuAddress1, float fieldOfView, const Location& target);

	void destruct(SharedResources* sharedResources);
	~MainCamera();

	void update(SharedResources* const sharedResources, const Location& target);
};