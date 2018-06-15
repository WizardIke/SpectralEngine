#include "CaveZone1.h"
#include <BaseExecutor.h>
#include "../Assets.h"
#include <ID3D12ResourceMapFailedException.h>
#include "../TextureNames.h"
#include "../MeshNames.h"
#include <TextureManager.h>
#include <D3D12DescriptorHeap.h>
#include <Light.h>
#include <VirtualTextureManager.h>
class BaseExecutor;

#include "CaveModelPart1.h"
#include "../Shaders/VtFeedbackMaterialPS.h"

namespace Cave
{
	class HDResources
	{
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;
		constexpr static unsigned int numComponents = 3u;

		static void componentUploaded(void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(requester);
			zone->componentUploaded(executor, numComponents);
		}

		D3D12Resource perObjectConstantBuffers;
		uint8_t* perObjectConstantBuffersCpuAddress;
	public:
		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS stone4FeedbackBufferPs;

		CaveModelPart1 caveModelPart1;


		HDResources(Executor* const executor, SharedResources& sr, void* zone) :
			light(DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
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
			Assets& sharedResources = reinterpret_cast<Assets&>(sr);
			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw ID3D12ResourceMapFailedException();
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();
			auto cpuConstantBuffer = perObjectConstantBuffersCpuAddress;

			stone4FeedbackBufferPs = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += vtFeedbackMaterialPsSize;
			cpuConstantBuffer += vtFeedbackMaterialPsSize;

			new(&caveModelPart1) CaveModelPart1(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			VirtualTextureManager::loadTexture(executor, sharedResources, TextureNames::stone04, { zone , [](void* requester, BaseExecutor* executor,
				SharedResources& sr, const VirtualTextureManager::Texture& texture)
			{
				auto& sharedResources = reinterpret_cast<Assets&>(sr);
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				const auto cpuStartAddress = resources->perObjectConstantBuffersCpuAddress;
				const auto gpuStartAddress = resources->perObjectConstantBuffers->GetGPUVirtualAddress();
				resources->caveModelPart1.setDiffuseTexture(texture.descriptorIndex, cpuStartAddress, gpuStartAddress);

				//stone4FeedbackBufferPs = create virtual feedback materialPS
				auto& textureInfo = sharedResources.virtualTextureManager.texturesByID[texture.textureID];
				auto stone4FeedbackBufferPsCpu = reinterpret_cast<VtFeedbackMaterialPS*>(cpuStartAddress + (resources->stone4FeedbackBufferPs - gpuStartAddress));
				stone4FeedbackBufferPsCpu->virtualTextureID1 = (float)(texture.textureID << 8u);
				stone4FeedbackBufferPsCpu->virtualTextureID2And3 = (float)0xffff;
				stone4FeedbackBufferPsCpu->textureHeightInPages = (float)textureInfo.heightInPages;
				stone4FeedbackBufferPsCpu->textureWidthInPages = (float)textureInfo.widthInPages;
				stone4FeedbackBufferPsCpu->usefulTextureHeight = (float)(textureInfo.height);
				stone4FeedbackBufferPsCpu->usefulTextureWidth = (float)(textureInfo.width);

				componentUploaded(requester, executor, sharedResources);
			} });

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::squareWithNormals, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				const auto resources = ((HDResources*)zone->newData);
				resources->caveModelPart1.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) &
				~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);

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
			pointLightConstantBufferCpuAddress->lightDirection.y = 0.f;
			pointLightConstantBufferCpuAddress->lightDirection.z = 1.f;
			pointLightConstantBufferCpuAddress->pointLightCount = 0u;
		}

		~HDResources() {}

		void update2(BaseExecutor* const executor1, SharedResources& sr)
		{
			Assets& sharedResources = (Assets&)sr;
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			uint32_t frameIndex = sharedResources.graphicsEngine.frameIndex;
			const auto commandList = executor->renderPass.colorSubPass().opaqueCommandList();
			const auto vtFeedbackCommandList = executor->renderPass.virtualTextureFeedbackSubPass().firstCommandList();
			

			if (caveModelPart1.isInView(sharedResources.mainCamera().frustum()))
			{
				if (sharedResources.renderPass.virtualTextureFeedbackSubPass().isInView(sharedResources))
				{
					vtFeedbackCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					vtFeedbackCommandList->SetPipelineState(sharedResources.pipelineStateObjects.vtFeedbackWithNormalsTwoSided);
					vtFeedbackCommandList->SetGraphicsRootConstantBufferView(3u, stone4FeedbackBufferPs);

					caveModelPart1.renderVirtualFeedback(vtFeedbackCommandList);
				}

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->SetPipelineState(sharedResources.pipelineStateObjects.directionalLightVtTwoSided);
				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress);
				caveModelPart1.render(commandList);
			}
		}

		void update1(BaseExecutor* const executor, SharedResources& sharedResources) {}

		static void create(void*const zone1, BaseExecutor*const exe, SharedResources& sharedResources)
		{
			const auto executor = reinterpret_cast<Executor*>(exe);
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			zone->newData = malloc(sizeof(HDResources));
			new(zone->newData) HDResources(executor, sharedResources, zone);
			componentUploaded(zone, executor, sharedResources);
		}

		void destruct(BaseExecutor*const executor, SharedResources& sr)
		{
			Assets& sharedResources = reinterpret_cast<Assets&>(sr);
			auto& meshManager = sharedResources.meshManager;
			auto& virtualTextureManager = sharedResources.virtualTextureManager;

			virtualTextureManager.unloadTexture(TextureNames::stone04, sharedResources);

			meshManager.unloadMesh(MeshNames::squareWithNormals, executor);
		}
	};

	static void update2(void* requester, BaseExecutor* executor, SharedResources& sharedResources);
	static void update1(void* requester, BaseExecutor* executor, SharedResources& sharedResources)
	{
		auto zone = reinterpret_cast<BaseZone*>(requester);
		zone->lastUsedData = zone->currentData;
		zone->lastUsedState = zone->currentState;

		switch (zone->currentState)
		{
		case 0u:
			reinterpret_cast<HDResources*>(zone->currentData)->update1(executor, sharedResources);
			executor->renderJobQueue().push(Job(zone, update2));
			break;
		}
	}

	static void update2(void* requester, BaseExecutor* executor, SharedResources& sharedResources)
	{
		auto zone = reinterpret_cast<BaseZone*>(requester);

		switch (zone->lastUsedState)
		{
		case 0u:
			reinterpret_cast<HDResources*>(zone->lastUsedData)->update2(executor, sharedResources);
			break;
		}

		executor->updateJobQueue().push(Job(zone, update1));
	}

	struct Zone1Functions
	{
		static void createNewStateData(BaseZone* zone, BaseExecutor* executor, SharedResources& sharedResources)
		{
			switch (zone->newState)
			{
			case 0u:
				sharedResources.backgroundQueue.push(Job(zone, &HDResources::create));
				break;
			}
		}

		static void deleteOldStateData(BaseZone* zone, BaseExecutor* executor, SharedResources& sharedResources)
		{
			switch (zone->oldState)
			{
			case 0u:
				sharedResources.backgroundQueue.push(Job(zone, [](void*const highDetailResource, BaseExecutor*const executor, SharedResources& sharedResources)
				{
					const auto zone = reinterpret_cast<BaseZone*const>(highDetailResource);
					auto resource = reinterpret_cast<HDResources*>(zone->oldData);
					resource->destruct(executor, sharedResources);
					resource->~HDResources();
					free(resource);
					zone->finishedDeletingOldState(executor);
				}));
				break;
			}
		}

		static void loadConnectedAreas(BaseZone* const zone, BaseExecutor* const executor, SharedResources& sharedResources, float distance, Area::VisitedNode* loadedAreas)
		{
			Assets* const assets = (Assets*)&sharedResources;
			assets->areas.outside.load(executor, sharedResources, Vector2{ 0.0f, 91.0f }, distance, loadedAreas);
		}

		static bool changeArea(BaseZone* const zone, BaseExecutor* const executor, SharedResources& sharedResources)
		{
			Assets* const assets = (Assets*)&sharedResources;
			auto& playerPosition = assets->playerPosition.location.position;
			if ((playerPosition.x - 74.0f) * (playerPosition.x - 74.0f) + (playerPosition.y + 10.0f) * (playerPosition.y + 10.0f) + (playerPosition.z - 71.0f) * (playerPosition.z - 71.0f) < 400)
			{
				assets->areas.cave.setAsCurrentArea(executor, sharedResources);
				return true;
			}

			return false;
		}

		static void start(BaseZone* const zone, BaseExecutor* const executor, SharedResources& sharedResources)
		{
			executor->updateJobQueue().push(Job(zone, update1));
		}
	};

	BaseZone Zone1()
	{
		return BaseZone::create<Zone1Functions>(1u);
	}
}