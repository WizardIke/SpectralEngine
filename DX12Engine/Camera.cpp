#include "Camera.h"
#include "SharedResources.h"

Camera::Camera(SharedResources* sharedResources, ID3D12Resource* image, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthSencilView,
	unsigned int width, unsigned int height, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress1, uint8_t*& constantBufferCpuAddress1, float fieldOfView,
	const Location& target) :
	width(width),
	height(height),
	image(image),
	renderTargetView(renderTargetView),
	depthSencilView(depthSencilView)
{
	constantBufferCpuAddress = reinterpret_cast<ConstantBuffer*>(constantBufferCpuAddress1);
	constantBufferCpuAddress1 += constantBufferPerObjectAlignedSize * frameBufferCount;
	constantBufferGpuAddress = constantBufferGpuAddress1;
	constantBufferGpuAddress1 += constantBufferPerObjectAlignedSize * frameBufferCount;

	mLocation.position = target.position;
	mLocation.rotation = target.rotation;

	DirectX::XMVECTOR positionVector = XMLoadFloat3(&mLocation.position);
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(mLocation.rotation.x, mLocation.rotation.y, mLocation.rotation.z);

	DirectX::XMFLOAT3 temp(0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR lookAtVector = DirectX::XMVectorAdd(positionVector, XMVector3TransformCoord(DirectX::XMLoadFloat3(&temp), rotationMatrix));

	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f };
	DirectX::XMVECTOR upVector = XMVector3TransformCoord(DirectX::XMLoadFloat3(&up), rotationMatrix);

	DirectX::XMMATRIX mViewMatrix;
	mViewMatrix = DirectX::XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

	const float screenAspect = static_cast<float>(width) / static_cast<float>(height);
	mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	ConstantBuffer* constantBuffer = reinterpret_cast<ConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + sharedResources->graphicsEngine.frameIndex * constantBufferPerObjectAlignedSize);
	constantBuffer->viewProjectionMatrix = mViewMatrix * mProjectionMatrix;
	constantBuffer->cameraPosition = mLocation.position;

	mFrustum.update(mProjectionMatrix, mViewMatrix, screenNear, screenDepth);
}

Camera::~Camera() {}

void Camera::update(SharedResources* sharedResources, const DirectX::XMMATRIX& mViewMatrix)
{
	const auto constantBuffer = reinterpret_cast<ConstantBuffer*>(reinterpret_cast<unsigned char*>(constantBufferCpuAddress) + sharedResources->graphicsEngine.frameIndex * constantBufferPerObjectAlignedSize);
	constantBuffer->viewProjectionMatrix = mViewMatrix * mProjectionMatrix;;
	constantBuffer->cameraPosition = mLocation.position;

	mFrustum.update(mProjectionMatrix, mViewMatrix, screenNear, screenDepth);
}

static D3D12_VIEWPORT getViewPort(unsigned int width, unsigned int height)
{
	D3D12_VIEWPORT viewPort;
	viewPort.Height = static_cast<FLOAT>(height);
	viewPort.Width = static_cast<FLOAT>(width);
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;
	viewPort.TopLeftX = 0.0f;
	viewPort.TopLeftY = 0.0f;
	return viewPort;
}

static D3D12_RECT getScissorRect(unsigned int width, unsigned int height)
{
	D3D12_RECT scissorRect;
	scissorRect.top = 0;
	scissorRect.left = 0;
	scissorRect.bottom = height;
	scissorRect.right = width;
	return scissorRect;
}

void Camera::bind(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	auto frameIndex = sharedResources->graphicsEngine.frameIndex;
	auto viewPort = getViewPort(width, height);
	auto scissorRect = getScissorRect(width, height);
	auto constantBufferGPU = constantBufferGpuAddress + constantBufferPerObjectAlignedSize * frameIndex;
	for (auto start = first; start != end; ++start)
	{
		auto commandList = *start;
		commandList->RSSetViewports(1u, &viewPort);
		commandList->RSSetScissorRects(1u, &scissorRect);
		commandList->SetGraphicsRootConstantBufferView(0u, constantBufferGPU);
	}

	auto commandList = *first;
	commandList->OMSetRenderTargets(1u, &renderTargetView, TRUE, &depthSencilView);
}

void Camera::bindFirstThread(SharedResources* sharedResources, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	bind(sharedResources, first, end);
	auto commandList = *first;
	constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(renderTargetView, clearColor, 0u, nullptr);
	commandList->ClearDepthStencilView(depthSencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0u, nullptr);
}

ID3D12Resource* Camera::getImage()
{
	return image;
}

void Camera::makeReflectionLocation(Location& location, float height)
{
	location.position.x = mLocation.position.x;
	location.position.y = height + height - mLocation.position.y;
	location.position.z = mLocation.position.z;

	location.rotation.x = -mLocation.rotation.x;
	location.rotation.y = mLocation.rotation.y;
	location.rotation.z = mLocation.rotation.z;
}