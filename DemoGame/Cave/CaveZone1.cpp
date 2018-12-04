#include "CaveZone1.h"
#include "../ThreadResources.h"
#include "../GlobalResources.h"
#include <ID3D12ResourceMapFailedException.h>
#include "../TextureNames.h"
#include "../MeshNames.h"
#include <TextureManager.h>
#include <D3D12DescriptorHeap.h>
#include <Light.h>
#include <VirtualTextureManager.h>
#include "../StreamingRequests.h"

#include "CaveModelPart1.h"
#include "../Shaders/VtFeedbackMaterialPS.h"

namespace Cave
{
	class HDResources
	{
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;
		constexpr static unsigned int numComponents = 3u;

		static void componentUploaded(Zone<ThreadResources, GlobalResources>* zone, ThreadResources& executor, GlobalResources&)
		{
			zone->componentUploaded(executor, numComponents);
		}

		D3D12Resource perObjectConstantBuffers;
		unsigned char* perObjectConstantBuffersCpuAddress;
	public:
		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS stone4FeedbackBufferPs;

		CaveModelPart1 caveModelPart1;


		HDResources(ThreadResources& threadResources, GlobalResources& globalResources, Zone<ThreadResources, GlobalResources>& zone) :
			light(DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
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

			stone4FeedbackBufferPs = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += vtFeedbackMaterialPsSize;
			cpuConstantBuffer += vtFeedbackMaterialPsSize;

			new(&caveModelPart1) CaveModelPart1(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
			VirtualTextureRequest* stone04Request = new VirtualTextureRequest([](VirtualTextureManager::TextureStreamingRequest& request, void* tr, void* gr, const VirtualTextureManager::Texture& texture)
			{
				ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
				auto& zone = reinterpret_cast<VirtualTextureRequest&>(request).zone;
				auto resources = ((HDResources*)zone.newData);
				const auto cpuStartAddress = resources->perObjectConstantBuffersCpuAddress;
				const auto gpuStartAddress = resources->perObjectConstantBuffers->GetGPUVirtualAddress();
				resources->caveModelPart1.setDiffuseTexture(texture.descriptorIndex, cpuStartAddress, gpuStartAddress);

				//stone4FeedbackBufferPs = create virtual feedback materialPS
				auto& textureInfo = globalResources.virtualTextureManager.texturesByID[texture.textureID];
				auto stone4FeedbackBufferPsCpu = reinterpret_cast<VtFeedbackMaterialPS*>(cpuStartAddress + (resources->stone4FeedbackBufferPs - gpuStartAddress));
				stone4FeedbackBufferPsCpu->virtualTextureID1 = (float)(texture.textureID << 8u);
				stone4FeedbackBufferPsCpu->virtualTextureID2And3 = (float)0xffff;
				stone4FeedbackBufferPsCpu->textureHeightInPages = (float)textureInfo.heightInPages;
				stone4FeedbackBufferPsCpu->textureWidthInPages = (float)textureInfo.widthInPages;
				stone4FeedbackBufferPsCpu->usefulTextureHeight = (float)(textureInfo.height);
				stone4FeedbackBufferPsCpu->usefulTextureWidth = (float)(textureInfo.width);

				componentUploaded(&zone, threadResources, globalResources);
			}, [](StreamingRequest* request, void*, void*)
			{
				delete static_cast<VirtualTextureRequest*>(request);
			}, TextureNames::stone04, zone);
			virtualTextureManager.loadTexture(threadResources, globalResources, stone04Request);

			MeshManager& meshManager = globalResources.meshManager;
			MeshRequest* squareWithNormalsRequest = new MeshRequest([](MeshManager::MeshStreamingRequest& request, void* tr, void* gr, Mesh& mesh)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
				auto& zone = static_cast<MeshRequest&>(request).zone;
				const auto resources = ((HDResources*)zone.newData);
				resources->caveModelPart1.mesh = &mesh;
				componentUploaded(&zone, threadResources, globalResources);
			}, [](StreamingRequest* request, void*, void*)
			{
				delete static_cast<MeshRequest*>(request);
			}, MeshNames::squareWithNormals, zone);
			meshManager.loadMeshWithPositionTextureNormal(threadResources, globalResources, squareWithNormalsRequest);

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

		void update2(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			const auto commandList = threadResources.renderPass.colorSubPass().opaqueCommandList();
			const auto vtFeedbackCommandList = threadResources.renderPass.virtualTextureFeedbackSubPass().firstCommandList();
			

			if (caveModelPart1.isInView(globalResources.mainCamera().frustum()))
			{
				if (globalResources.renderPass.virtualTextureFeedbackSubPass().isInView())
				{
					vtFeedbackCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					vtFeedbackCommandList->SetPipelineState(globalResources.pipelineStateObjects.vtFeedbackWithNormalsTwoSided);
					vtFeedbackCommandList->SetGraphicsRootConstantBufferView(3u, stone4FeedbackBufferPs);

					caveModelPart1.renderVirtualFeedback(vtFeedbackCommandList);
				}

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->SetPipelineState(globalResources.pipelineStateObjects.directionalLightVtTwoSided);
				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress);
				caveModelPart1.render(commandList);
			}
		}

		void update1(ThreadResources&, GlobalResources&) {}

		static void create(void* zone1, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& zone = *reinterpret_cast<Zone<ThreadResources, GlobalResources>*>(zone1);
			zone.newData = malloc(sizeof(HDResources));
			new(zone.newData) HDResources(threadResources, globalResources, zone);
			componentUploaded(&zone, threadResources, globalResources);
		}

		void destruct(ThreadResources&, GlobalResources& globalResources)
		{
			auto& meshManager = globalResources.meshManager;
			auto& virtualTextureManager = globalResources.virtualTextureManager;

			virtualTextureManager.unloadTexture(TextureNames::stone04, globalResources);

			meshManager.unloadMesh(MeshNames::squareWithNormals);
		}
	};

	static void update2(void* requester, ThreadResources& threadResources, GlobalResources& globalResources);
	static void update1(void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		auto& zone = *reinterpret_cast<Zone<ThreadResources, GlobalResources>*>(requester);
		zone.lastUsedData = zone.currentData;
		zone.lastUsedState = zone.currentState;

		switch (zone.currentState)
		{
		case 0u:
			reinterpret_cast<HDResources*>(zone.currentData)->update1(threadResources, globalResources);
			threadResources.taskShedular.update2NextQueue().push({ &zone, update2 });
			break;
		}
	}

	static void update2(void* requester, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		auto& zone = *reinterpret_cast<Zone<ThreadResources, GlobalResources>*>(requester);

		switch (zone.lastUsedState)
		{
		case 0u:
			reinterpret_cast<HDResources*>(zone.lastUsedData)->update2(threadResources, globalResources);
			break;
		}
		threadResources.taskShedular.update1NextQueue().push({ &zone, update1 });
	}

	struct Zone1Functions
	{
		static void createNewStateData(Zone<ThreadResources, GlobalResources>& zone, ThreadResources& threadResources, GlobalResources&)
		{
			switch (zone.newState)
			{
			case 0u:
				threadResources.taskShedular.backgroundQueue().push({ &zone, &HDResources::create });
				break;
			}
		}

		static void deleteOldStateData(Zone<ThreadResources, GlobalResources>& zone, ThreadResources& threadResources, GlobalResources&)
		{
			switch (zone.oldState)
			{
			case 0u:
				threadResources.taskShedular.backgroundQueue().push({ &zone, [](void* context, ThreadResources& threadResources, GlobalResources& globalResources)
				{
					auto& zone = *reinterpret_cast<Zone<ThreadResources, GlobalResources>*>(context);
					auto resource = reinterpret_cast<HDResources*>(zone.oldData);
					resource->destruct(threadResources, globalResources);
					resource->~HDResources();
					free(resource);
					zone.finishedDeletingOldState(threadResources);
				} });
				break;
			}
		}

		static void loadConnectedAreas(Zone<ThreadResources, GlobalResources>&, ThreadResources& threadResources, GlobalResources& globalResources, float distance, Area::VisitedNode* loadedAreas)
		{
			globalResources.areas.outside.load(threadResources, globalResources, Vector2{ 0.0f, 91.0f }, distance, loadedAreas);
		}

		static bool changeArea(Zone<ThreadResources, GlobalResources>&, ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto& playerPosition = globalResources.playerPosition.location.position;
			if ((playerPosition.x - 74.0f) * (playerPosition.x - 74.0f) + (playerPosition.y + 10.0f) * (playerPosition.y + 10.0f) + (playerPosition.z - 71.0f) * (playerPosition.z - 71.0f) < 400)
			{
				globalResources.areas.cave.setAsCurrentArea(threadResources, globalResources);
				return true;
			}

			return false;
		}

		static void start(Zone<ThreadResources, GlobalResources>& zone, ThreadResources& threadResources, GlobalResources&)
		{
			threadResources.taskShedular.update1NextQueue().push({ &zone, update1 });
		}
	};

	Zone<ThreadResources, GlobalResources> Zone1()
	{
		return Zone<ThreadResources, GlobalResources>::create<Zone1Functions>(1u);
	}
}