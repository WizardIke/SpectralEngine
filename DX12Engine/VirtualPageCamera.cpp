#include "VirtualPageCamera.h"
#include "CameraUtil.h"

VirtualPageCamera::VirtualPageCamera(ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
	unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
	Transform& target) :
	mWidth(width),
	mHeight(height),
	mImage(image),
	renderTargetView(renderTargetView),
	depthSencilView(depthSencilView)
{
	constantBufferCpuAddress = reinterpret_cast<VtFeedbackCameraMaterial*>(constantBufferCpuAddress1);
	constantBufferCpuAddress1 += bufferSizePS * frameBufferCount;
	constantBufferGpuAddress = constantBufferGpuAddress1;
	constantBufferGpuAddress1 += bufferSizePS * frameBufferCount;

	mTransform = &target;
	DirectX::XMMATRIX mViewMatrix = mTransform->toMatrix();

	const float screenAspect = static_cast<float>(width) / static_cast<float>(height);
	mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		auto constantBuffer = reinterpret_cast<VtFeedbackCameraMaterial*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + i * bufferSizePS);
		constantBuffer->viewProjectionMatrix = mViewMatrix * mProjectionMatrix;
		constantBuffer->feedbackBias = 0u;
	}
}

VirtualPageCamera::~VirtualPageCamera() {}