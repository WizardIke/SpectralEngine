#include "VirtualPageCamera.h"
#include "CameraUtil.h"
#include "GraphicsEngine.h"

VirtualPageCamera::VirtualPageCamera(ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
	unsigned int width, unsigned int height, const Transform& target, float fieldOfView) :
	mWidth(width),
	mHeight(height),
	mImage(image),
	renderTargetView(renderTargetView),
	depthSencilView(depthSencilView),
	mTransform(target),
	mProjectionMatrix(DirectX::XMMatrixPerspectiveFovLH(fieldOfView, /*screenAspect*/static_cast<float>(width) / static_cast<float>(height), screenNear, screenDepth)),
	fieldOfView(fieldOfView)
{}

void VirtualPageCamera::setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, unsigned char*& constantBufferCpuAddress1)
{
	constantBufferCpuAddress = reinterpret_cast<VtFeedbackCameraMaterial*>(constantBufferCpuAddress1);
	constantBufferCpuAddress1 += bufferSizePS * frameBufferCount;
	constantBufferGpuAddress = constantBufferGpuAddress1;
	constantBufferGpuAddress1 += bufferSizePS * frameBufferCount;
}

void VirtualPageCamera::resize(ID3D12Resource* image, unsigned int width, unsigned int height)
{
	mImage = image;
	mWidth = width;
	mHeight = height;
	mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, /*screenAspect*/static_cast<float>(width) / static_cast<float>(height), screenNear, screenDepth);
}

void VirtualPageCamera::beforeRender(const GraphicsEngine& graphicsEngine, float mipBias)
{
	const auto constantBuffer = reinterpret_cast<VtFeedbackCameraMaterial*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + graphicsEngine.frameIndex * bufferSizePS);
	constantBuffer->viewProjectionMatrix = mTransform.toMatrix() * mProjectionMatrix;
	constantBuffer->feedbackBias = mipBias;
}