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
#include <MeshManager.h>

template<unsigned int x, unsigned int z>
class TestZoneFunctions
{
	class HDResources
	{
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;
		constexpr static unsigned int numComponents = 3u;

		static void componentUploaded(void* requester, BaseExecutor* executor)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(requester);
			BaseZone::componentUploaded<BaseZone::high, HDResources::numComponents>(zone, executor, ((HDResources*)zone->nextResources)->numComponentsLoaded);
		}
	public:
		std::atomic<unsigned char> numComponentsLoaded = 0u;

		Mesh* meshes[numMeshes];
		D3D12Resource perObjectConstantBuffers;

		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;

		HighResPlane<x, z> highResPlaneModel;


		HDResources(BaseExecutor* const executor, void* zone) :
			light(DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
			perObjectConstantBuffers(executor->sharedResources->graphicsEngine.graphicsDevice, []()
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
			resourceDesc.Width = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; //might need changing
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
			unsigned int stone04 = TextureManager::loadTexture(TextureNames::stone04, zone, executor, componentUploaded);

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::HighResMesh1, zone, executor, componentUploaded, &meshes[0]);

			uint8_t* perObjectConstantBuffersCpuAddress;
			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw ID3D12ResourceMapFailedException();
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & ~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);

			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(perObjectConstantBuffersCpuAddress);
			perObjectConstantBuffersCpuAddress += pointLightConstantBufferAlignedSize;

			pointLightConstantBufferGpuAddress = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += pointLightConstantBufferAlignedSize;

			pointLightConstantBufferCpuAddress->ambientLight.x = 0.2f;
			pointLightConstantBufferCpuAddress->ambientLight.y = 0.2f;
			pointLightConstantBufferCpuAddress->ambientLight.z = 0.2f;
			pointLightConstantBufferCpuAddress->ambientLight.w = 1.0f;
			pointLightConstantBufferCpuAddress->directionalLight.x = 2.0f;
			pointLightConstantBufferCpuAddress->directionalLight.y = 2.0f;
			pointLightConstantBufferCpuAddress->directionalLight.z = 2.0f;
			pointLightConstantBufferCpuAddress->directionalLight.w = 1.0f;
			pointLightConstantBufferCpuAddress->lightDirection.x = 0.f;
			pointLightConstantBufferCpuAddress->lightDirection.y = -0.7f;
			pointLightConstantBufferCpuAddress->lightDirection.z = 0.7f;
			pointLightConstantBufferCpuAddress->pointLightCount = 0u;

			new(&highResPlaneModel) HighResPlane<x, z>(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, stone04);
		}

		~HDResources() {}

		void update1(BaseExecutor* const executor) {}

		void update2(BaseExecutor* const executor1)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			const auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			const auto commandList = executor->renderPass.colorSubPass().opaqueCommandList();
			Assets* const assets = (Assets*)executor->sharedResources;

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (highResPlaneModel.isInView(assets->mainFrustum))
			{
				commandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);

				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress);
				commandList->SetGraphicsRootConstantBufferView(2u, highResPlaneModel.vsPerObjectCBVGpuAddress);
				commandList->SetGraphicsRootConstantBufferView(3u, highResPlaneModel.psPerObjectCBVGpuAddress);
				commandList->IASetVertexBuffers(0u, 1u, &meshes[highResPlaneModel.meshIndex]->vertexBufferView);
				commandList->IASetIndexBuffer(&meshes[highResPlaneModel.meshIndex]->indexBufferView);
				commandList->DrawIndexedInstanced(meshes[highResPlaneModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}
		}

		static void create(void*const zone1, BaseExecutor*const executor)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			zone->nextResources = malloc(sizeof(HDResources));
			new(zone->nextResources) HDResources(executor, zone);
			componentUploaded(zone, executor);
		}

		void destruct(BaseExecutor*const executor)
		{
			const auto sharedResources = executor->sharedResources;
			auto& textureManager = sharedResources->textureManager;
			auto& meshManager = sharedResources->meshManager;

			textureManager.unloadTexture(TextureNames::stone04, executor);

			meshManager.unloadMesh(MeshNames::HighResMesh1, executor);
		}
	};

	static void restart(BaseZone* zone, BaseExecutor* const executor)
	{
		auto oldLevelOfDetail = zone->levelOfDetail.load(std::memory_order::memory_order_seq_cst);
		if (oldLevelOfDetail == 0u)
		{
			executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(zone1);
				reinterpret_cast<HDResources* const>(zone->currentResources)->update1(executor);
				executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
				{
					const auto zone = reinterpret_cast<BaseZone* const>(zone1);
					reinterpret_cast<HDResources* const>(zone->currentResources)->update2(executor);
					restart(zone, executor);
				}));
			}));
		}
		else if (oldLevelOfDetail == 1u)
		{
			executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				executor->renderJobQueue().push(Job(zone1, [](void*const zone1, BaseExecutor*const executor)
				{
					const auto zone = reinterpret_cast<BaseZone* const>(zone1);
					restart(zone, executor);
				}));
			}));
		}
		else if (oldLevelOfDetail == 2u)
		{
			executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				executor->renderJobQueue().push(Job(zone1, [](void*const zone1, BaseExecutor*const executor)
				{
					const auto zone = reinterpret_cast<BaseZone* const>(zone1);
					restart(zone, executor);
				}));
			}));
		}
	}
	static void update1(BaseZone* zone, BaseExecutor* const executor)
	{
		restart(zone, executor);
	}
	static void update2(BaseZone* zone, BaseExecutor* const executor)
	{
		executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(zone1);
			restart(zone, executor);
		}));
	}
public:
	static void loadHighDetailJobs(BaseZone* zone, BaseExecutor* const executor)
	{
		executor->sharedResources->backgroundQueue.push(Job(zone, &HDResources::create));
	}
	static void loadMediumDetailJobs(BaseZone* zone, BaseExecutor* const executor)
	{
		zone->lastComponentLoaded<BaseZone::medium>(executor);
	}
	static void loadLowDetailJobs(BaseZone* zone, BaseExecutor* const executor)
	{
		zone->lastComponentLoaded<BaseZone::low>(executor);
	}

	static void unloadHighDetailJobs(BaseZone* zone, BaseExecutor* const executor)
	{
		executor->sharedResources->backgroundQueue.push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			const auto resource = reinterpret_cast<HDResources*>(zone->nextResources);
			resource->~HDResources();
			free(zone->nextResources);
			zone->nextResources = nullptr;
			zone->lastComponentUnloaded<BaseZone::high>(executor);
		}));
	}
	static void unloadMediumDetailJobs(BaseZone* zone, BaseExecutor* const executor)
	{
		zone->lastComponentUnloaded<BaseZone::medium>(executor);
	}
	static void unloadLowDetailJobs(BaseZone* zone, BaseExecutor* const executor)
	{
		zone->lastComponentUnloaded<BaseZone::low>(executor);
	}

	static void start(BaseZone* zone, BaseExecutor* const executor)
	{
		auto sharedResources = executor->sharedResources;
		sharedResources->syncMutex.lock();
		if (sharedResources->nextPhaseJob == Executor::update1NextPhaseJob)
		{
			sharedResources->syncMutex.unlock();
			update2(zone, executor);
		}
		else
		{
			sharedResources->syncMutex.unlock();
			update1(zone, executor);
		}
	}

	static void loadConnectedAreas(BaseZone* zone, BaseExecutor* const executor, float distanceSquared, Area::VisitedNode* loadedAreas) {}
	static bool changeArea(BaseZone* zone, BaseExecutor* const executor) { return false; }
};

template<unsigned int x, unsigned int z>
static BaseZone TestZone()
{
	return BaseZone::create<TestZoneFunctions<x, z>>();
}