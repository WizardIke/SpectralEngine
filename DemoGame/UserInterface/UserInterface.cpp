#include "UserInterface.h"
#include <DirectXMath.h>
#include "../ThreadResources.h"
#include "../GlobalResources.h"

UserInterface::UserInterface(GlobalResources& globalResources) :
	UserInterface(globalResources, static_cast<float>(globalResources.window.width()) / static_cast<float>(globalResources.window.height()))
{}

UserInterface::UserInterface(GlobalResources& globalResources, float aspectRatio) :
	CPUUsageSentence(globalResources.graphicsEngine.graphicsDevice, DirectX::XMFLOAT2(-0.98f, 1.0f - 0.01f * aspectRatio), DirectX::XMFLOAT2(0.1f, 0.1f * aspectRatio),
		DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)),
	FPSSentence(globalResources.graphicsEngine.graphicsDevice, DirectX::XMFLOAT2(-0.98f, 1.0f - 0.12f * aspectRatio), DirectX::XMFLOAT2(0.1f, 0.1f * aspectRatio)),
	globalResources(globalResources)
{
	const VirtualPageCamera& camera = (*globalResources.renderPass.virtualTextureFeedbackSubPass().cameras().begin());
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
	resourceDesc.Height = camera.height();
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.MipLevels = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	resourceDesc.Width = camera.width();

	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 0u;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.VisibleNodeMask = 0u;
	virtualFeedbackTextureCopy = D3D12Resource(globalResources.graphicsEngine.graphicsDevice, heapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

#ifndef NDEBUG
	virtualFeedbackTextureCopy->SetName(L"User interface copy of virtual feedback texture");
#endif

	descriptorIndex = globalResources.graphicsEngine.descriptorAllocator.allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE descritporHandle = globalResources.graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	descritporHandle.ptr += globalResources.graphicsEngine.cbvAndSrvAndUavDescriptorSize * descriptorIndex;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1u;
	srvDesc.Texture2D.MostDetailedMip = 0u;
	srvDesc.Texture2D.PlaneSlice = 0u;
	srvDesc.Texture2D.ResourceMinLODClamp = 0u;
	globalResources.graphicsEngine.graphicsDevice->CreateShaderResourceView(virtualFeedbackTextureCopy, &srvDesc, descritporHandle);
}

UserInterface::~UserInterface()
{
	globalResources.graphicsEngine.descriptorAllocator.deallocate(descriptorIndex);
}

void UserInterface::update1()
{
	float frameTime = globalResources.timer.frameTime();
	CPUUsageSentence.update(frameTime);
	FPSSentence.update(frameTime);
}

void UserInterface::update2(ThreadResources& threadResources)
{
	auto frameIndex = globalResources.graphicsEngine.frameIndex;
	CPUUsageSentence.beforeRender(frameIndex);
	FPSSentence.beforeRender(frameIndex);

	const auto opaqueDirectCommandList = threadResources.renderPass.colorSubPass().opaqueCommandList();
	
	opaqueDirectCommandList->SetPipelineState(globalResources.pipelineStateObjects.text);
	opaqueDirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, globalResources.arial.getConstantBuffer());

	opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &CPUUsageSentence.textVertexBufferView[frameIndex]);
	opaqueDirectCommandList->DrawInstanced(4u, static_cast<UINT>(CPUUsageSentence.text.length()), 0u, 0u);

	opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &FPSSentence.textVertexBufferView[frameIndex]);
	opaqueDirectCommandList->DrawInstanced(4u, static_cast<UINT>(FPSSentence.text.length()), 0u, 0u);

	if (displayVirtualFeedbackTexture)
	{
		auto& subPass = globalResources.renderPass.virtualTextureFeedbackSubPass();
		if (subPass.isInView())
		{
			auto image = &(*globalResources.renderPass.virtualTextureFeedbackSubPass().cameras().begin()).getImage();

			D3D12_RESOURCE_BARRIER barrier[2];
			barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[0].Transition.pResource = virtualFeedbackTextureCopy;
			barrier[0].Transition.Subresource = 0u;
			barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;

			opaqueDirectCommandList->ResourceBarrier(1u, barrier);

			opaqueDirectCommandList->CopyResource(virtualFeedbackTextureCopy, image);

			barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
			barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[1].Transition.pResource = image;
			barrier[1].Transition.Subresource = 0u;
			barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
			barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

			opaqueDirectCommandList->ResourceBarrier(2u, barrier);
		}
		
		opaqueDirectCommandList->SetPipelineState(globalResources.pipelineStateObjects.vtDebugDraw);
		opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, bufferGpu);
		opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, bufferGpu + TexturedQuadMaterialVsSize);
		opaqueDirectCommandList->DrawInstanced(4u, 1u, 0u, 0u);
	}
}

void UserInterface::setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress)
{
	bufferGpu = constantBufferGpuAddress;
	constantBufferGpuAddress += BasicMaterialPsSize + TexturedQuadMaterialVsSize;
	auto BufferVsCpu = reinterpret_cast<TexturedQuadMaterialVS*>(constantBufferCpuAddress);
	constantBufferCpuAddress += TexturedQuadMaterialVsSize;
	auto bufferPSCpu = reinterpret_cast<BasicMaterialPS*>(constantBufferCpuAddress);
	constantBufferCpuAddress += BasicMaterialPsSize;

	BufferVsCpu->pos[0] = -1.0f;
	BufferVsCpu->pos[1] = 0.0f;
	BufferVsCpu->pos[2] = 1.0f;
	BufferVsCpu->pos[3] = 1.0f;
	BufferVsCpu->texCoords[0] = 0.0f;
	BufferVsCpu->texCoords[1] = 0.0f;
	BufferVsCpu->texCoords[2] = 1.0f;
	BufferVsCpu->texCoords[3] = 1.0f;


	bufferPSCpu->baseColorTexture = descriptorIndex;
}

void UserInterface::start(ThreadResources& threadResources)
{
	threadResources.taskShedular.pushPrimaryTask(0u, {this, [](void* requester, ThreadResources& threadResources)
	{
		UserInterface* const ui = static_cast<UserInterface*>(requester);
		auto stopRequest = ui->mStopRequest;
		if(stopRequest != nullptr)
		{
			stopRequest->callback(*stopRequest, &threadResources);
			return;
		}
		ui->update1();
		threadResources.taskShedular.pushPrimaryTask(1u, { requester, [](void*const requester, ThreadResources& threadResources)
		{
			UserInterface* const ui = static_cast<UserInterface*>(requester);
			ui->update2(threadResources);
			ui->start(threadResources);
		} });
	}});
}

void UserInterface::stop(StopRequest& stopRequest, ThreadResources& threadResources)
{
	stopRequest.stopRequest = &mStopRequest;
	threadResources.taskShedular.pushPrimaryTask(1u, {&stopRequest, [](void* requester, ThreadResources&)
	{
		StopRequest& stopRequest = *static_cast<StopRequest*>(requester);
		*stopRequest.stopRequest = &stopRequest;
	}});
}