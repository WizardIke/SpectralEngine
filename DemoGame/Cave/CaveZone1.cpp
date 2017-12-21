#include "CaveZone1.h"
#include <BaseExecutor.h>
#include "../Assets.h"
#include <ID3D12ResourceMapFailedException.h>
#include "../TextureNames.h"
#include "../MeshNames.h"
#include <TextureManager.h>
#include <D3D12DescriptorHeap.h>
#include <Light.h>
class BaseExecutor;

#include "CaveModelPart1.h"

namespace Cave
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

		CaveModelPart1 caveModelPart1;


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
			unsigned int stone02 = TextureManager::loadTexture(TextureNames::stone02, zone, executor, componentUploaded);

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::squareWithNormals, zone, executor, componentUploaded, &meshes[0]);

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
			pointLightConstantBufferCpuAddress->directionalLight.x = 0.2f;
			pointLightConstantBufferCpuAddress->directionalLight.y = 0.2f;
			pointLightConstantBufferCpuAddress->directionalLight.z = 0.2f;
			pointLightConstantBufferCpuAddress->directionalLight.w = 1.0f;
			pointLightConstantBufferCpuAddress->lightDirection.x = 0.f;
			pointLightConstantBufferCpuAddress->lightDirection.y = 0.f;
			pointLightConstantBufferCpuAddress->lightDirection.z = 1.f;
			pointLightConstantBufferCpuAddress->pointLightCount = 0u;

			new(&caveModelPart1) CaveModelPart1(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, stone02);
		}

		~HDResources() {}

		void update2(BaseExecutor* const executor1)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			uint32_t frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			const auto commandList = executor->renderPass.colorSubPass().opaqueCommandList();
			Assets* const assets = (Assets*)executor->sharedResources;

			if (caveModelPart1.isInView(assets->mainFrustum))
			{
				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);

				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress);

				caveModelPart1.render(commandList, meshes);
			}
		}

		void update1(BaseExecutor* const executor) {}

		static void create(void*const zone1, BaseExecutor*const executor)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			zone->nextResources = malloc(sizeof(HDResources));
			new(zone->nextResources) HDResources(executor, zone);
			componentUploaded(zone, executor);
		}

		void destruct(BaseExecutor*const executor)
		{
			auto& sharedResources = executor->sharedResources;
			auto& textureManager = sharedResources->textureManager;
			auto& meshManager = sharedResources->meshManager;

			textureManager.unloadTexture(TextureNames::stone04, executor);

			meshManager.unloadMesh(MeshNames::squareWithNormals, executor);
		}
	};

	void restart(BaseZone* const zone, BaseExecutor* const executor)
	{
		auto oldLevelOfDetail = zone->levelOfDetail.load(std::memory_order::memory_order_seq_cst);
		if (oldLevelOfDetail == 0u)
		{
			executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone*const>(zone1);
				reinterpret_cast<HDResources*>(zone->currentResources)->update1(executor);
				executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
				{
					const auto zone = reinterpret_cast<BaseZone*const>(zone1);
					reinterpret_cast<HDResources*>(zone->currentResources)->update2(executor);
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
					const auto zone = reinterpret_cast<BaseZone*const>(zone1);
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
					const auto zone = reinterpret_cast<BaseZone*const>(zone1);
					restart(zone, executor);
				}));
			}));
		}
	}
	void update1(BaseZone* const zone, BaseExecutor* const executor)
	{
		restart(zone, executor);
	}

	void update2(BaseZone* const zone, BaseExecutor* const executor)
	{
		executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			restart(zone, executor);
		}));
	}

	struct Zone1Functions
	{
		static void loadHighDetailJobs(BaseZone* const zone, BaseExecutor* const executor)
		{
			executor->sharedResources->backgroundQueue.push(Job(zone, &HDResources::create));
		}
		static void loadMediumDetailJobs(BaseZone* const zone, BaseExecutor* const executor)
		{
			zone->lastComponentLoaded<BaseZone::medium>(executor);
		}
		static void loadLowDetailJobs(BaseZone* const zone, BaseExecutor* const executor)
		{
			zone->lastComponentLoaded<BaseZone::low>(executor);
		}

		static void unloadHighDetailJobs(BaseZone* const zone, BaseExecutor* const executor)
		{
			executor->sharedResources->backgroundQueue.push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone*const>(zone1);
				auto resource = reinterpret_cast<HDResources*>(zone->nextResources);
				resource->~HDResources();
				free(zone->nextResources);
				zone->nextResources = nullptr;
				zone->lastComponentUnloaded<BaseZone::high>(executor);
			}));
		}
		static void unloadMediumDetailJobs(BaseZone* const zone, BaseExecutor* const executor)
		{
			zone->lastComponentUnloaded<BaseZone::medium>(executor);
		}
		static void unloadLowDetailJobs(BaseZone* const zone, BaseExecutor* const executor)
		{
			zone->lastComponentUnloaded<BaseZone::low>(executor);
		}

		static void loadConnectedAreas(BaseZone* const zone, BaseExecutor* const executor, float distanceSquared, Area::VisitedNode* loadedAreas)
		{
			Assets* const assets = (Assets*)executor->sharedResources;
			assets->areas.outside.load(executor, Vector2{ 0.0f, 91.0f }, std::sqrt(distanceSquared), loadedAreas);
		}
		static bool changeArea(BaseZone* const zone, BaseExecutor* const executor)
		{
			Assets* const assets = (Assets*)executor->sharedResources;
			auto& playerPosition = assets->playerPosition.location.position;
			if ((playerPosition.x - 74.0f) * (playerPosition.x - 74.0f) + (playerPosition.y + 10.0f) * (playerPosition.y + 10.0f) + (playerPosition.z - 71.0f) * (playerPosition.z - 71.0f) < 400)
			{
				assets->areas.cave.setAsCurrentArea(executor);
				return true;
			}

			return false;
		}

		static void start(BaseZone* const zone, BaseExecutor* const executor)
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
	};

	BaseZone Zone1()
	{
		return BaseZone::create<Zone1Functions>();
	}
}