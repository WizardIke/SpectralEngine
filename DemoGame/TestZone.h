#pragma once
#include <BaseZone.h>
#include <Job.h>
#include "Executor.h"
#include "Assets.h"
#include "HighResPlane.h"
#include <Light.h>
#include "TextureNames.h"
#include "MeshNames.h"
#include "PipelineStateObjects.h"
#include <ID3D12ResourceMapFailedException.h>
#include <TextureManager.h>
#include <VirtualTextureManager.h>
#include <MeshManager.h>

#include "Shaders/VtFeedbackMaterialPS.h"

template<unsigned int x, unsigned int z>
class TestZoneFunctions
{
	class HDResources
	{
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;
		constexpr static unsigned int numComponents = 1u + numMeshes + numTextures;

		static void componentUploaded(void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(requester);
			BaseZone::componentUploaded<BaseZone::high, BaseZone::high>(zone, executor, sharedResources, ((HDResources*)zone->nextResources)->numComponentsLoaded, numComponents);
		}

		std::atomic<unsigned char> numComponentsLoaded = 0u;
		D3D12Resource perObjectConstantBuffers;
		uint8_t* perObjectConstantBuffersCpuAddress;
	public:
		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS stone4FeedbackBufferPs;

		HighResPlane<x, z> highResPlaneModel;


		HDResources(Executor* const executor, SharedResources& sr, void* zone) :
			light(DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f), DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
			perObjectConstantBuffers(sr.graphicsEngine.graphicsDevice, []()
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
		}(), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr)
		{
			auto& sharedResources = reinterpret_cast<Assets&>(sr);

			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw ID3D12ResourceMapFailedException();
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();
			auto cpuConstantBuffer = perObjectConstantBuffersCpuAddress;

			new(&highResPlaneModel) HighResPlane<x, z>(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(cpuConstantBuffer);
			cpuConstantBuffer += vtFeedbackMaterialPsSize;

			stone4FeedbackBufferPs = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += vtFeedbackMaterialPsSize;

			VirtualTextureManager::loadTexture(executor, sharedResources, TextureNames::stone04, { zone , [](void* requester, BaseExecutor* executor,
				SharedResources& sr, const VirtualTextureManager::Texture& texture)
			{
				auto& sharedResources = reinterpret_cast<Assets&>(sr);
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->nextResources);
				const auto cpuStartAddress = resources->perObjectConstantBuffersCpuAddress;
				const auto gpuStartAddress = resources->perObjectConstantBuffers->GetGPUVirtualAddress();
				resources->highResPlaneModel.setDiffuseTexture(texture.descriptorIndex, cpuStartAddress, gpuStartAddress);

				//stone4FeedbackBufferPs = create virtual feedback materialPS
				auto& textureInfo = sharedResources.virtualTextureManager.texturesByID.data()[texture.textureID];
				auto stone4FeedbackBufferPsCpu = reinterpret_cast<VtFeedbackMaterialPS*>(cpuStartAddress + (resources->stone4FeedbackBufferPs - gpuStartAddress));
				stone4FeedbackBufferPsCpu->virtualTextureID1 = (float)(texture.textureID << 8u);
				stone4FeedbackBufferPsCpu->virtualTextureID2And3 = (float)0xffff;
				stone4FeedbackBufferPsCpu->textureHeightInPages = (float)textureInfo.heightInPages;
				stone4FeedbackBufferPsCpu->textureWidthInPages = (float)textureInfo.widthInPages;
				stone4FeedbackBufferPsCpu->usefulTextureHeight = (float)textureInfo.heightInTexels;
				stone4FeedbackBufferPsCpu->usefulTextureWidth = (float)textureInfo.widthInTexels;

				componentUploaded(requester, executor, sharedResources);
			} });

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::HighResMesh1, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				const auto resources = ((HDResources*)zone->nextResources);
				resources->highResPlaneModel.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});

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

		void update1(BaseExecutor* const executor, SharedResources& sharedResources) {}

		void update2(BaseExecutor* const executor1, SharedResources& sr)
		{
			auto& sharedResources = reinterpret_cast<Assets&>(sr);
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			const auto frameIndex = sharedResources.graphicsEngine.frameIndex;
			const auto commandList = executor->renderPass.colorSubPass().opaqueCommandList();
			const auto vtFeedbackCommandList = executor->renderPass.virtualTextureFeedbackSubPass().firstCommandList();
			Assets* const assets = (Assets*)&sharedResources;

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (highResPlaneModel.isInView(assets->mainCamera.frustum()))
			{
				if (sharedResources.renderPass.virtualTextureFeedbackSubPass().isInView(sharedResources))
				{
					vtFeedbackCommandList->SetPipelineState(assets->pipelineStateObjects.vtFeedbackWithNormals);
					vtFeedbackCommandList->IASetVertexBuffers(0u, 1u, &highResPlaneModel.mesh->vertexBufferView);
					vtFeedbackCommandList->IASetIndexBuffer(&highResPlaneModel.mesh->indexBufferView);
					vtFeedbackCommandList->SetGraphicsRootConstantBufferView(2u, highResPlaneModel.vsBufferGpu());
					vtFeedbackCommandList->SetGraphicsRootConstantBufferView(3u, stone4FeedbackBufferPs);
					vtFeedbackCommandList->DrawIndexedInstanced(highResPlaneModel.mesh->indexCount, 1u, 0u, 0, 0u);
				}

				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress);
				commandList->SetPipelineState(assets->pipelineStateObjects.directionalLightVt);
				commandList->IASetVertexBuffers(0u, 1u, &highResPlaneModel.mesh->vertexBufferView);
				commandList->IASetIndexBuffer(&highResPlaneModel.mesh->indexBufferView);
				commandList->SetGraphicsRootConstantBufferView(2u, highResPlaneModel.vsBufferGpu());
				commandList->SetGraphicsRootConstantBufferView(3u, highResPlaneModel.psBufferGpu());
				commandList->DrawIndexedInstanced(highResPlaneModel.mesh->indexCount, 1u, 0u, 0, 0u);
			}
		}

		static void create(void*const zone1, BaseExecutor*const exe, SharedResources& sharedResources)
		{
			const auto executor = reinterpret_cast<Executor* const>(exe);
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			zone->nextResources = malloc(sizeof(HDResources));
			new(zone->nextResources) HDResources(executor, sharedResources, zone);
			componentUploaded(zone, executor, sharedResources);
		}

		void destruct(BaseExecutor*const executor, SharedResources& sr)
		{
			auto& sharedResources = reinterpret_cast<Assets&>(sr);
			auto& meshManager = sharedResources.meshManager;

			//textureManager.unloadTexture(TextureNames::stone04, executor);
			VirtualTextureManager::unloadTexture(TextureNames::stone04, sharedResources);

			meshManager.unloadMesh(MeshNames::HighResMesh1, executor);
		}
	};

	static void update1HighDetail(BaseZone* const zone, BaseExecutor* const executor)
	{
		executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(zone1);
			reinterpret_cast<HDResources*>(zone->currentResources)->update1(executor, sharedResources);
			executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor, SharedResources& sharedResources)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(zone1);
				reinterpret_cast<HDResources*>(zone->currentResources)->update2(executor, sharedResources);
				restart(zone, executor, sharedResources);
			}));
		}));
	}

	static void restart(BaseZone* const zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		auto oldLevelOfDetail = zone->levelOfDetail.load(std::memory_order::memory_order_acquire);
		switch (oldLevelOfDetail)
		{
		case BaseZone::high:
			update1HighDetail(zone, executor);
			break;
		case BaseZone::transitionHighToMedium:
		{
			zone->transition<BaseZone::high, BaseZone::medium>(executor, sharedResources);
			break;
		}
		case BaseZone::transitionHighToLow:
			zone->transition<BaseZone::high, BaseZone::low>(executor, sharedResources);
			break;
		case BaseZone::transitionHighToUnloaded:
			zone->transition<BaseZone::high, BaseZone::unloaded>(executor, sharedResources);
			break;
		}
	}

	static void update1(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		restart(zone, executor, sharedResources);
	}
	static void update2(BaseZone* zone, BaseExecutor* const executor)
	{
		executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(zone1);
			restart(zone, executor, sharedResources);
		}));
	}
public:
	static void loadHighDetailJobs(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		sharedResources.backgroundQueue.push(Job(zone, &HDResources::create));
	}
	static void loadMediumDetailJobs(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		zone->lastComponentLoaded<BaseZone::medium, BaseZone::high>(executor, sharedResources);
	}
	static void loadLowDetailJobs(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		zone->lastComponentLoaded<BaseZone::low, BaseZone::high>(executor, sharedResources);
	}

	static void unloadHighDetailJobs(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		sharedResources.backgroundQueue.push(Job(zone, [](void*const zone1, BaseExecutor*const executor, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			const auto resource = reinterpret_cast<HDResources*>(zone->nextResources);
			resource->~HDResources();
			free(zone->nextResources);
			zone->nextResources = nullptr;
			zone->lastComponentUnloaded<BaseZone::high>(executor, sharedResources);
		}));
	}
	static void unloadMediumDetailJobs(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		zone->lastComponentUnloaded<BaseZone::high>(executor, sharedResources);
	}
	static void unloadLowDetailJobs(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		zone->lastComponentUnloaded<BaseZone::high>(executor, sharedResources);
	}

	static void start(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
	{
		sharedResources.syncMutex.lock();
		if (sharedResources.nextPhaseJob == Executor::update1NextPhaseJob)
		{
			sharedResources.syncMutex.unlock();
			update2(zone, executor);
		}
		else
		{
			sharedResources.syncMutex.unlock();
			update1(zone, executor, sharedResources);
		}
	}

	static void loadConnectedAreas(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources, float distanceSquared, Area::VisitedNode* loadedAreas) {}
	static bool changeArea(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources) { return false; }
};

template<unsigned int x, unsigned int z>
static BaseZone TestZone()
{
	return BaseZone::create<TestZoneFunctions<x, z>>();
}