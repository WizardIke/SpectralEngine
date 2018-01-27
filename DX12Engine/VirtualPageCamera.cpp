#include "VirtualPageCamera.h"
#include "SharedResources.h"
#include "CameraUtil.h"

VirtualPageCamera::VirtualPageCamera(SharedResources* sharedResources, ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
	unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
	Transform& target) :
	width(width),
	height(height),
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

void VirtualPageCamera::update(SharedResources* sharedResources, float mipBias)
{
	const auto constantBuffer = reinterpret_cast<VtFeedbackCameraMaterial*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + sharedResources->graphicsEngine.frameIndex * bufferSizePS);
	DirectX::XMMATRIX mViewMatrix = mTransform->toMatrix();
	constantBuffer->viewProjectionMatrix = mViewMatrix * mProjectionMatrix;;
	constantBuffer->feedbackBias = mipBias;
}

void VirtualPageCamera::bind(SharedResources& sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	auto frameIndex = sharedResources.graphicsEngine.frameIndex;
	auto constantBufferGPU = constantBufferGpuAddress + bufferSizePS * frameIndex;
	CameraUtil::bind(first, end, CameraUtil::getViewPort(width, height), CameraUtil::getScissorRect(width, height), constantBufferGPU, &renderTargetView, &depthSencilView);
}

void VirtualPageCamera::bindFirstThread(SharedResources& sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	bind(sharedResources, first, end);
	auto commandList = *first;
	constexpr float clearColor[] = { 65535.0f, 65280.0f, 0.0f, 0.0f };
	commandList->ClearRenderTargetView(renderTargetView, clearColor, 0u, nullptr);
	commandList->ClearDepthStencilView(depthSencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, (uint8_t)0u, 0u, nullptr);
}