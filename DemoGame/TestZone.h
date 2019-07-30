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

		static void componentUploaded(Zone<ThreadResources>& zone)
		{
			auto& globalResources = *static_cast<GlobalResources*>(zone.context);
			zone.componentUploaded(globalResources.taskShedular, numComponents);
		}

		D3D12Resource perObjectConstantBuffers;
		unsigned char* perObjectConstantBuffersCpuAddress;
	public:
		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS stone4FeedbackBufferPs;

		HighResPlane<x, z> highResPlaneModel;


		HDResources(ThreadResources& threadResources, GlobalResources& globalResources, Zone<ThreadResources>& zone) :
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

			highResPlaneModel.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			stone4FeedbackBufferPs = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += vtFeedbackMaterialPsSize;
			cpuConstantBuffer += vtFeedbackMaterialPsSize;

			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
			VirtualTextureRequest* stone04Request = new VirtualTextureRequest([](VirtualTextureManager::TextureStreamingRequest& request, void*, const VirtualTextureManager::Texture& texture)
			{
				auto& zone = static_cast<VirtualTextureRequest&>(request).zone;
				auto resources = ((HDResources*)zone.newData);
				const auto cpuStartAddress = resources->perObjectConstantBuffersCpuAddress;
				const auto gpuStartAddress = resources->perObjectConstantBuffers->GetGPUVirtualAddress();
				resources->highResPlaneModel.setDiffuseTexture(texture.descriptorIndex, cpuStartAddress, gpuStartAddress);

				//stone4FeedbackBufferPs = create virtual feedback materialPS
				auto stone4FeedbackBufferPsCpu = reinterpret_cast<VtFeedbackMaterialPS*>(cpuStartAddress + (resources->stone4FeedbackBufferPs - gpuStartAddress));
				stone4FeedbackBufferPsCpu->virtualTextureID1 = (float)(texture.textureID << 8u);
				stone4FeedbackBufferPsCpu->virtualTextureID2And3 = (float)0xffff;
				stone4FeedbackBufferPsCpu->textureHeightInPages = (float)texture.heightInPages;
				stone4FeedbackBufferPsCpu->textureWidthInPages = (float)texture.widthInPages;
				stone4FeedbackBufferPsCpu->usefulTextureHeight = (float)(texture.height);
				stone4FeedbackBufferPsCpu->usefulTextureWidth = (float)(texture.width);

				delete static_cast<VirtualTextureRequest*>(&request);
				componentUploaded(zone);
			}, TextureNames::stone04, zone);
			virtualTextureManager.load(stone04Request, threadResources);

			MeshManager& meshManager = globalResources.meshManager;
			MeshRequest* HighResMesh1Request = new MeshRequest([](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
			{
				auto& zone = static_cast<MeshRequest&>(request).zone;
				auto resources = ((HDResources*)zone.newData);
				resources->highResPlaneModel.mesh = &mesh;

				delete static_cast<MeshRequest*>(&request);
				componentUploaded(zone);
			}, MeshNames::HighResMesh1, zone);
			meshManager.load(HighResMesh1Request, threadResources);

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

		static void create(void* context, ThreadResources& threadResources)
		{
			auto& zone = *static_cast<Zone<ThreadResources>*>(context);
			auto& globalResources = *static_cast<GlobalResources*>(zone.context);
			zone.newData = operator new(sizeof(HDResources));
			new(zone.newData) HDResources(threadResources, globalResources, zone);
			componentUploaded(zone);
		}

		class HdUnloader : private VirtualTextureManager::UnloadRequest, private MeshManager::UnloadRequest
		{
			static constexpr unsigned int numberOfComponentsToUnload = 2u;
			std::atomic<unsigned int> numberOfComponentsUnloaded = 0u;
			Zone<ThreadResources>& zone;

			void componentUnloaded(void*)
			{
				if (numberOfComponentsUnloaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponentsToUnload - 1u))
				{
					auto& zoneRef = zone;
					delete this;
					auto resource = static_cast<HDResources*>(zoneRef.oldData);
					resource->~HDResources();
					operator delete(resource);
					auto& globalResources = *static_cast<GlobalResources*>(zoneRef.context);
					zoneRef.finishedDeletingOldState(globalResources.taskShedular);
				}
			}

			HdUnloader(Zone<ThreadResources>& zone1) :
				VirtualTextureManager::UnloadRequest(TextureNames::stone04, [](AsynchronousFileManager::ReadRequest& unloader, void* tr)
					{
						static_cast<HdUnloader&>(static_cast<VirtualTextureManager::UnloadRequest&>(unloader)).componentUnloaded(tr);
					}),
				MeshManager::UnloadRequest(MeshNames::HighResMesh1, [](AsynchronousFileManager::ReadRequest& unloader, void* tr)
					{
						static_cast<HdUnloader&>(static_cast<MeshManager::UnloadRequest&>(unloader)).componentUnloaded(tr);
					}),
						zone(zone1)
					{}
		public:
			static HdUnloader* create(Zone<ThreadResources>& zone1)
			{
				return new HdUnloader(zone1);
			}

			void unload(VirtualTextureManager& textureManager, MeshManager& meshManager, ThreadResources& threadResources)
			{
				textureManager.unload(this, threadResources);
				meshManager.unload(this, threadResources);
			}
		};

		static void destruct(Zone<ThreadResources>& zone, ThreadResources& threadResources)
		{
			auto& globalResources = *static_cast<GlobalResources*>(zone.context);
			MeshManager& meshManager = globalResources.meshManager;
			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;

			auto unloader = HdUnloader::create(zone);
			unloader->unload(virtualTextureManager, meshManager, threadResources);
		}
	};

	static void update1(void* context, ThreadResources& threadResources)
	{
		auto& zone = *static_cast<Zone<ThreadResources>*>(context);
		auto& globalResources = *static_cast<GlobalResources*>(zone.context);

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

	static void update2(void* context, ThreadResources& threadResources)
	{
		auto& zone = *static_cast<Zone<ThreadResources>*>(context);
		auto& globalResources = *static_cast<GlobalResources*>(zone.context);

		switch (zone.lastUsedState)
		{
		case 0u:
			static_cast<HDResources*>(zone.lastUsedData)->update2(threadResources, globalResources);
			break;
		}
		threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1 });
	}
public:
	static void createNewStateData(Zone<ThreadResources>& zone, ThreadResources& threadResources)
	{
		switch (zone.newState)
		{
		case 0u:
			threadResources.taskShedular.pushBackgroundTask({ &zone, &HDResources::create });
			break;
		}
	}

	static void deleteOldStateData(Zone<ThreadResources>& zone, ThreadResources&)
	{
		switch (zone.oldState)
		{
		case 0u:
			GraphicsEngine& graphicsEngine = static_cast<GlobalResources*>(zone.context)->graphicsEngine;
			zone.executeWhenGpuFinishesCurrentFrame(graphicsEngine, [](LinkedTask& task, void* tr)
			{
				auto& zone = Zone<ThreadResources>::from(task);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				threadResources.taskShedular.pushBackgroundTask({ &zone, [](void* context, ThreadResources& threadResources)
				{
					auto& zone = *static_cast<Zone<ThreadResources>*>(context);
					HDResources::destruct(zone, threadResources);
				} });
			});
			break;
		}
	}
	
	static void start(Zone<ThreadResources>& zone, ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1 });
	}

	static Range<Portal*> getPortals(Zone<ThreadResources>&)
	{
		return Range<Portal*>(nullptr, nullptr);
	}
};

template<unsigned int x, unsigned int z>
static Zone<ThreadResources> TestZone(void* context)
{
	return Zone<ThreadResources>::create<TestZoneFunctions<x, z>>(1u, context);
}