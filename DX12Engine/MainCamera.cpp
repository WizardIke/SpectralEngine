#include "MainCamera.h"
#include "BaseExecutor.h"
#include "SharedResources.h"

struct CameraConstantBuffer
{
	DirectX::XMMATRIX viewProjectionMatrix;
	DirectX::XMFLOAT3 cameraPosition;
};

constexpr static unsigned int constantBufferPerObjectAlignedSize = (sizeof(CameraConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);

MainCamera::MainCamera(D3D12GraphicsEngine& graphicsEngine, Window& window, D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, float fieldOfView) :
	renderTargetViewDescriptorHeap(graphicsEngine.graphicsDevice, []()
{
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.NumDescriptors = frameBufferCount;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	renderTargetViewHeapDesc.NodeMask = 0;
	return renderTargetViewHeapDesc;
}())
{
	cameraConstantBufferCpuAddress = reinterpret_cast<CameraConstantBuffer*>(constantBufferCpuAddress);
	constantBufferCpuAddress += constantBufferPerObjectAlignedSize * frameBufferCount;
	cameraConstantBufferGpuAddress = constantBufferGpuAddress;
	constantBufferGpuAddress += constantBufferPerObjectAlignedSize * frameBufferCount;



	D3D12_CPU_DESCRIPTOR_HANDLE backbufferRenderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto i = 0u; i < frameBufferCount; ++i)
	{
		graphicsEngine.graphicsDevice->CreateRenderTargetView(window.getBuffer(i), nullptr, backbufferRenderTargetViewHandle);
		backbufferRenderTargetViewHandle.ptr += graphicsEngine.renderTargetViewDescriptorSize;
	}

	const float screenAspect = static_cast<float>(window.width()) / static_cast<float>(window.height());
	projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);
}

MainCamera::~MainCamera() {}

void MainCamera::update(BaseExecutor* const executor, const Location& target)
{
	location.position = target.position;
	location.rotation = target.rotation;

	DirectX::XMVECTOR positionVector = XMLoadFloat3(&location.position);
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(location.rotation.x, location.rotation.y, location.rotation.z);

	DirectX::XMFLOAT3 temp(0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR lookAtVector = DirectX::XMVectorAdd(positionVector, XMVector3TransformCoord(DirectX::XMLoadFloat3(&temp), rotationMatrix));

	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f };
	DirectX::XMVECTOR upVector = XMVector3TransformCoord(DirectX::XMLoadFloat3(&up), rotationMatrix);

	viewMatrix = DirectX::XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

	auto sharedResources = executor->sharedResources;
	CameraConstantBuffer* cameraConstantBuffer = reinterpret_cast<CameraConstantBuffer*>(reinterpret_cast<unsigned char*>(cameraConstantBufferCpuAddress) + executor->sharedResources->graphicsEngine.frameIndex * constantBufferPerObjectAlignedSize);
	cameraConstantBuffer->viewProjectionMatrix = viewMatrix * projectionMatrix;;
	cameraConstantBuffer->cameraPosition = location.position;
}

static D3D12_VIEWPORT getViewPort(const Window& window)
{
	D3D12_VIEWPORT viewPort;
	viewPort.Height = static_cast<FLOAT>(window.height());
	viewPort.Width = static_cast<FLOAT>(window.width());
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;
	viewPort.TopLeftX = 0.0f;
	viewPort.TopLeftY = 0.0f;
	return viewPort;
}

static D3D12_RECT getScissorRect(const Window& window)
{
	D3D12_RECT scissorRect;
	scissorRect.top = 0;
	scissorRect.left = 0;
	scissorRect.bottom = window.height();
	scissorRect.right = window.width();
	return scissorRect;
}

void MainCamera::bind(SharedResources& sharedResources, uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	auto viewPort = getViewPort(sharedResources.window);
	auto scissorRect = getScissorRect(sharedResources.window);
	for (auto start = first; start != end; ++start)
	{
		auto commandList = *start;
		commandList->RSSetViewports(1u, &viewPort);
		commandList->RSSetScissorRects(1u, &scissorRect);
		commandList->SetGraphicsRootConstantBufferView(0u, cameraConstantBufferGpuAddress + constantBufferPerObjectAlignedSize * frameIndex);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRenderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	backBufferRenderTargetViewHandle.ptr += frameIndex * sharedResources.graphicsEngine.renderTargetViewDescriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilViewHandle = sharedResources.graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	auto commandList = *first;
	commandList->OMSetRenderTargets(1u, &backBufferRenderTargetViewHandle, TRUE, &depthSencilViewHandle);
	constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
}

void MainCamera::bindFirstThread(SharedResources& sharedResources, uint32_t frameIndex, ID3D12GraphicsCommandList** first, ID3D12GraphicsCommandList** end)
{
	auto viewPort = getViewPort(sharedResources.window);
	auto scissorRect = getScissorRect(sharedResources.window);
	for (auto start = first; start != end; ++start)
	{
		auto commandList = *start;
		commandList->RSSetViewports(1u, &viewPort);
		commandList->RSSetScissorRects(1u, &scissorRect);
		commandList->SetGraphicsRootConstantBufferView(0u, cameraConstantBufferGpuAddress + constantBufferPerObjectAlignedSize * frameIndex);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRenderTargetViewHandle = renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	backBufferRenderTargetViewHandle.ptr += frameIndex * sharedResources.graphicsEngine.renderTargetViewDescriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSencilViewHandle = sharedResources.graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	auto commandList = *first;
	commandList->OMSetRenderTargets(1u, &backBufferRenderTargetViewHandle, TRUE, &depthSencilViewHandle);
	constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	commandList->ClearRenderTargetView(backBufferRenderTargetViewHandle, clearColor, 0u, nullptr);
	commandList->ClearDepthStencilView(depthSencilViewHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0u, nullptr);
}

void MainCamera::makeReflectionMatrix(DirectX::XMMATRIX& ReflectionMatrix, float height)
{
	DirectX::XMVECTOR reflectedPosition = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(location.position.x, (height + height) - location.position.y, location.position.z));

	DirectX::XMFLOAT3 lookAt;
	lookAt.x = sinf(location.rotation.y) + location.position.x;
	lookAt.y = (height + height) - location.position.y;
	lookAt.z = cosf(location.rotation.y) + location.position.z;

	DirectX::XMVECTOR lookAtVector = DirectX::XMLoadFloat3(&lookAt);

	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f };
	ReflectionMatrix = DirectX::XMMatrixLookAtLH(reflectedPosition, lookAtVector, DirectX::XMLoadFloat3(&up));
}

ID3D12Resource* MainCamera::getImage(BaseExecutor* const executor)
{
	return executor->sharedResources->window.getBuffer(executor->sharedResources->graphicsEngine.frameIndex); 
}