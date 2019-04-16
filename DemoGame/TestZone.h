#pragma once
#include <Zone.h>
#include "ThreadResources.h"
#include "GlobalResources.h"
#include "HighResPlane.h"
#include <Light.h>
#include "TextureNames.h"
#include "MeshNames.h"
#include "PipelineStateObjects.h"
#include <ID3D12ResourceMapFailedException.h>
#include <TextureManager.h>
#include <VirtualTextureManager.h>
#include <MeshManager.h>
#include "StreamingRequests.h"

#include "Shaders/VtFeedbackMaterialPS.h"

template<unsigned int x, unsigned int z>
class TestZoneFunctions
{
	class HDResources
	{
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;
		constexpr static unsigned int numComponents = 3u;

		static void componentUploaded(void* requester, ThreadResources& executor, GlobalResources& globalResources)
		{
			const auto zone = static_cast<Zone<ThreadResources, GlobalResources>*>(requester);
			zone->componentUploaded(executor, globalResources, numComponents);
		}

		D3D12Resource perObjectConstantBuffers;
		unsigned char* perObjectConstantBuffersCpuAddress;
	public:
		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS stone4FeedbackBufferPs;

		HighResPlane<x, z> highResPlaneModel;


		HDResources(ThreadResources& threadResources, GlobalResources& globalResources, Zone<ThreadResources, GlobalResources>& zone) :
			light(DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f), DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
			perObjectConstantBuffers(globalResources.graphicsEngine.graphicsDevice, []()
		{
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.CreationNodeMask = 1u;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.VisibleNodeMask = 1u;
			return heapProperties;
		}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, []()
		{
			D3D12_RESOURCE_DESC resourceDesc;
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resourceDesc.Width = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resourceDesc.Height = 1u;
			resourceDesc.DepthOrArraySize = 1u;
			resourceDesc.MipLevels = 1u;
			resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc = { 1u, 0u };
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			return resourceDesc;
		}(), D3D12_RESOURCE_STATE_GENERIC_READ)
		{
			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw ID3D12ResourceMapFailedException();
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();
			auto cpuConstantBuffer = perObjectConstantBuffersCpuAddress;

			new(&highResPlaneModel) HighResPlane<x, z>(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			stone4FeedbackBufferPs = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += vtFeedbackMaterialPsSize;
			cpuConstantBuffer += vtFeedbackMaterialPsSize;

			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
			VirtualTextureRequest* stone04Request = new VirtualTextureRequest([](VirtualTextureManager::TextureStreamingRequest& request, void* tr, void* gr, const VirtualTextureManager::Texture& texture)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				auto& zone = static_cast<VirtualTextureRequest&>(request).zone;
				auto resources = ((HDResources*)zone.newData);
				const auto cpuStartAddress = resources->perObjectConstantBuffersCpuAddress;
				const auto gpuStartAddress = resources->perObjectConstantBuffers->GetGPUVirtualAddress();
				resources->highResPlaneModel.setDiffuseTexture(texture.descriptorIndex, cpuStartAddress, gpuStartAddress);

				//stone4FeedbackBufferPs = create virtual feedback materialPS
				auto& textureInfo = globalResources.virtualTextureManager.texturesByID[texture.textureID];
				auto stone4FeedbackBufferPsCpu = reinterpret_cast<VtFeedbackMaterialPS*>(cpuStartAddress + (resources->stone4FeedbackBufferPs - gpuStartAddress));
				stone4FeedbackBufferPsCpu->virtualTextureID1 = (float)(texture.textureID << 8u);
				stone4FeedbackBufferPsCpu->virtualTextureID2And3 = (float)0xffff;
				stone4FeedbackBufferPsCpu->textureHeightInPages = (float)textureInfo.heightInPages;
				stone4FeedbackBufferPsCpu->textureWidthInPages = (float)textureInfo.widthInPages;
				stone4FeedbackBufferPsCpu->usefulTextureHeight = (float)(textureInfo.height);
				stone4FeedbackBufferPsCpu->usefulTextureWidth = (float)(textureInfo.width);

				delete static_cast<VirtualTextureRequest*>(&request);
				componentUploaded(&zone, threadResources, globalResources);
			}, TextureNames::stone04, zone);
			virtualTextureManager.load(stone04Request, threadResources, globalResources);

			MeshManager& meshManager = globalResources.meshManager;
			MeshRequest* HighResMesh1Request = new MeshRequest([](MeshManager::MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				auto& zone = static_cast<MeshRequest&>(request).zone;
				auto resources = ((HDResources*)zone.newData);
				resources->highResPlaneModel.mesh = &mesh;

				delete static_cast<MeshRequest*>(&request);
				componentUploaded(&zone, threadResources, globalResources);
			}, MeshNames::HighResMesh1, zone);
			meshManager.load(HighResMesh1Request, threadResources, globalResources);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & ~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);

			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(cpuConstantBuffer);
			cpuConstantBuffer += pointLightConstantBufferAlignedSize;

			pointLightConstantBufferGpuAddress = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += pointLightConstantBufferAlignedSize;

			pointLightConstantBufferCpuAddress->ambientLight.x = 0.2f;
			pointLightConstantBufferCpuAddress->ambientLight.y = 0.2f;
			pointLightConstantBufferCpuAddress->ambientLight.z = 0.2f;
			pointLightConstantBufferCpuAddress->ambientLight.w = 1.0f;
			pointLightConstantBufferCpuAddress->directionalLight.x = 1.0f;
			pointLightConstantBufferCpuAddress->directionalLight.y = 1.0f;
			pointLightConstantBufferCpuAddress->directionalLight.z = 1.0f;
			pointLightConstantBufferCpuAddress->directionalLight.w = 1.0f;
			pointLightConstantBufferCpuAddress->lightDirection.x = 0.f;
			pointLightConstantBufferCpuAddress->lightDirection.y = -0.7f;
			pointLightConstantBufferCpuAddress->lightDirection.z = 0.7f;
			pointLightConstantBufferCpuAddress->pointLightCount = 0u;
		}

		~HDResources() 
		{
			perObjectConstantBuffers->Unmap(0u, nullptr);
		}

		void update1(ThreadResources&, GlobalResources&) {}

		void update2(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			const auto frameIndex = globalResources.graphicsEngine.frameIndex;
			const auto commandList = threadResources.renderPass.colorSubPass().opaqueCommandList();
			const auto vtFeedbackCommandList = threadResources.renderPass.virtualTextureFeedbackSubPass().firstCommandList();

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (highResPlaneModel.isInView(globalResources.mainCamera().frustum()))
			{
				if (globalResources.renderPass.virtualTextureFeedbackSubPass().isInView())
				{
					vtFeedbackCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					vtFeedbackCommandList->SetPipelineState(globalResources.pipelineStateObjects.vtFeedbackWithNormals);
					vtFeedbackCommandList->IASetVertexBuffers(0u, 1u, &highResPlaneModel.mesh->vertexBufferView);
					vtFeedbackCommandList->IASetIndexBuffer(&highResPlaneModel.mesh->indexBufferView);
					vtFeedbackCommandList->SetGraphicsRootConstantBufferView(2u, highResPlaneModel.vsBufferGpu());
					vtFeedbackCommandList->SetGraphicsRootConstantBufferView(3u, stone4FeedbackBufferPs);
					vtFeedbackCommandList->DrawIndexedInstanced(highResPlaneModel.mesh->indexCount(), 1u, 0u, 0, 0u);
				}

				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress);
				commandList->SetPipelineState(globalResources.pipelineStateObjects.directionalLightVt);
				commandList->IASetVertexBuffers(0u, 1u, &highResPlaneModel.mesh->vertexBufferView);
				commandList->IASetIndexBuffer(&highResPlaneModel.mesh->indexBufferView);
				commandList->SetGraphicsRootConstantBufferView(2u, highResPlaneModel.vsBufferGpu());
				commandList->SetGraphicsRootConstantBufferView(3u, highResPlaneModel.psBufferGpu());
				commandList->DrawIndexedInstanced(highResPlaneModel.mesh->indexCount(), 1u, 0u, 0, 0u);
			}
		}

		static void create(void* context, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& zone = *static_cast<Zone<ThreadResources, GlobalResources>*>(context);
			zone.newData = malloc(sizeof(HDResources));
			new(zone.newData) HDResources(threadResources, globalResources, zone);
			componentUploaded(&zone, threadResources, globalResources);
		}

		void destruct(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			MeshManager& meshManager = globalResources.meshManager;
			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;

			auto unloadTexture = new VirtualTextureManager::Message(TextureNames::stone04, [](AsynchronousFileManager::ReadRequest& request, void*, void*)
			{
				delete static_cast<VirtualTextureManager::Message*>(&request);
			});
			virtualTextureManager.unload(unloadTexture, threadResources, globalResources);

			auto unloadMesh = new MeshManager::Message(MeshNames::HighResMesh1, [](AsynchronousFileManager::ReadRequest& request, void*, void*)
			{
				delete static_cast<MeshManager::Message*>(&request);
			});
			meshManager.unload(unloadMesh, threadResources, globalResources);
		}
	};

	static void update1(void* context, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		auto& zone = *static_cast<Zone<ThreadResources, GlobalResources>*>(context);
		zone.lastUsedData = zone.currentData;
		zone.lastUsedState = zone.currentState;

		switch (zone.currentState)
		{
		case 0u:
			static_cast<HDResources*>(zone.currentData)->update1(threadResources, globalResources);
			threadResources.taskShedular.pushPrimaryTask(1u, { &zone, update2 });
			break;
		}
	}

	static void update2(void* context, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		auto& zone = *static_cast<Zone<ThreadResources, GlobalResources>*>(context);

		switch (zone.lastUsedState)
		{
		case 0u:
			static_cast<HDResources*>(zone.lastUsedData)->update2(threadResources, globalResources);
			break;
		}
		threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1 });
	}
public:
	static void createNewStateData(Zone<ThreadResources, GlobalResources>& zone, ThreadResources& threadResources, GlobalResources&)
	{
		switch (zone.newState)
		{
		case 0u:
			threadResources.taskShedular.pushBackgroundTask({ &zone, &HDResources::create });
			break;
		}
	}

	static void deleteOldStateData(Zone<ThreadResources, GlobalResources>& zone, ThreadResources& threadResources, GlobalResources&)
	{
		switch (zone.oldState)
		{
		case 0u:
			threadResources.taskShedular.pushBackgroundTask({ &zone, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
			{
				auto& zone = *static_cast<Zone<ThreadResources, GlobalResources>*>(context);
				auto resource = static_cast<HDResources*>(zone.oldData);
				resource->destruct(threadResources, globalResources);
				resource->~HDResources();
				free(resource);
				zone.finishedDeletingOldState(threadResources, globalResources);
			} });
			break;
		}
	}
	
	static void start(Zone<ThreadResources, GlobalResources>& zone, ThreadResources& threadResources, GlobalResources&)
	{
		threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1 });
	}

	static Range<Portal*> getPortals(Zone<ThreadResources, GlobalResources>&)
	{
		return Range<Portal*>(nullptr, nullptr);
	}
};

template<unsigned int x, unsigned int z>
static Zone<ThreadResources, GlobalResources> TestZone()
{
	return Zone<ThreadResources, GlobalResources>::create<TestZoneFunctions<x, z>>(1u);
}