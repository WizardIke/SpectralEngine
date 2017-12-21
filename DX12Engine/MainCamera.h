#pragma once
#include "Camera.h"
#include "D3D12DescriptorHeap.h"

class MainCamera : public Camera
{
	D3D12DescriptorHeap renderTargetViewDescriptorHeap;
public:
	MainCamera() {}
	MainCamera(SharedResources* sharedResources, unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1,
		uint8_t*& constantBufferCpuAddress1, float fieldOfView, const Location& target);
	~MainCamera();

	void update(SharedResources* const sharedResources, const Location& target);
};