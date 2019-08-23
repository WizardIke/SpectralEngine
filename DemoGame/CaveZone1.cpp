#include "CaveZone1.h"
#include "ThreadResources.h"
#include "GlobalResources.h"
#include <ID3D12ResourceMapFailedException.h>
#include "Resources.h"
#include <TextureManager.h>
#include <D3D12DescriptorHeap.h>
#include <Light.h>
#include <VirtualTextureManager.h>
#include "StreamingRequests.h"

#include "Models/CaveModelPart1.h"

namespace Cave
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

		struct VtFeedbackMaterialPS
		{
			float virtualTextureID1;
			float virtualTextureID2And3;
			float textureWidthInPages;
			float textureHeightInPages;
			float usefulTextureWidth; //width of virtual texture not counting padding
			float usefulTextureHeight;
		};

		static constexpr std::size_t vtFeedbackMaterialPsSize = (sizeof(VtFeedbackMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);

		D3D12Resource perObjectConstantBuffers;
		unsigned char* perObjectConstantBuffersCpuAddress;
	public:
		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS stone4FeedbackBufferPs;

		CaveModelPart1 caveModelPart1;


		HDResources(ThreadResources& threadResources, GlobalResources& globalResources, Zone<ThreadResources>& zone) :
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

			caveModelPart1.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			VirtualTextureManager& virtualTextureManager = globalResources.virtualTextureManager;
			VirtualTextureRequest* stone04Request = new VirtualTextureRequest([](VirtualTextureManager::TextureStreamingRequest& request, void*, const VirtualTextureManager::Texture& texture)
			{
				auto& zone = static_cast<VirtualTextureRequest&>(request).zone;
				auto resources = ((HDResources*)zone.newData);
				const auto cpuStartAddress = resources->perObjectConstantBuffersCpuAddress;
				const auto gpuStartAddress = resources->perObjectConstantBuffers->GetGPUVirtualAddress();
				resources->caveModelPart1.setDiffuseTexture(texture.descriptorIndex, cpuStartAddress, gpuStartAddress);

				//stone4FeedbackBufferPs = create virtual feedback materialPS
				auto stone4FeedbackBufferPsCpu = reinterpret_cast<VtFeedbackMaterialPS*>(cpuStartAddress + (resources->stone4FeedbackBufferPs - gpuStartAddress));
				auto& textureInfo = *texture.info;
				stone4FeedbackBufferPsCpu->virtualTextureID1 = (float)(textureInfo.textureID << 8u);
				stone4FeedbackBufferPsCpu->virtualTextureID2And3 = (float)0xffff;
				stone4FeedbackBufferPsCpu->textureHeightInPages = (float)textureInfo.heightInPages;
				stone4FeedbackBufferPsCpu->textureWidthInPages = (float)textureInfo.widthInPages;
				stone4FeedbackBufferPsCpu->usefulTextureHeight = (float)(textureInfo.height);
				stone4FeedbackBufferPsCpu->usefulTextureWidth = (float)(textureInfo.width);

				delete static_cast<VirtualTextureRequest*>(&request);
				componentUploaded(zone);
			}, Resources::Textures::stone04Tiled, zone);
			virtualTextureManager.load(stone04Request, threadResources);

			MeshManager& meshManager = globalResources.meshManager;
			MeshRequest* squareWithNormalsRequest = new MeshRequest([](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
			{
				auto& zone = static_cast<MeshRequest&>(request).zone;
				const auto resources = ((HDResources*)zone.newData);
				resources->caveModelPart1.mesh = &mesh;

				delete static_cast<MeshRequest*>(&request);
				componentUploaded(zone);
			}, Resources::Meshes::squareWithNormals, zone);
			meshManager.load(squareWithNormalsRequest, threadResources);

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

		static void create(void* zone1, ThreadResources& threadResources)
		{
			auto& zone = *static_cast<Zone<ThreadResources>*>(zone1);
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
				VirtualTextureManager::UnloadRequest(Resources::Textures::stone04Tiled, [](AsynchronousFileManager::ReadRequest& unloader, void* tr)
					{
						static_cast<HdUnloader&>(static_cast<VirtualTextureManager::UnloadRequest&>(unloader)).componentUnloaded(tr);
					}),
				MeshManager::UnloadRequest(Resources::Meshes::squareWithNormals, [](AsynchronousFileManager::ReadRequest& unloader, void* tr)
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
			auto& meshManager = globalResources.meshManager;
			auto& virtualTextureManager = globalResources.virtualTextureManager;

			auto unloader = HdUnloader::create(zone);
			unloader->unload(virtualTextureManager, meshManager, threadResources);
		}
	};

	static void update2(void* requester, ThreadResources& threadResources);
	static void update1(void* requester, ThreadResources& threadResources)
	{
		auto& zone = *static_cast<Zone<ThreadResources>*>(requester);
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

	static void update2(void* requester, ThreadResources& threadResources)
	{
		auto& zone = *static_cast<Zone<ThreadResources>*>(requester);
		auto& globalResources = *static_cast<GlobalResources*>(zone.context);

		switch (zone.lastUsedState)
		{
		case 0u:
			static_cast<HDResources*>(zone.lastUsedData)->update2(threadResources, globalResources);
			break;
		}
		threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1 });
	}

	struct Zone1Functions
	{
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

		static Range<Portal*> getPortals(Zone<ThreadResources>&)
		{
			static Portal portals[1] = {Portal{Vector3(64.0f, 6.0f, 84.0f), Vector3(64.0f, 6.0f, 84.0f), 0u}};
			return range(portals, portals + 1u);
		}

		static void start(Zone<ThreadResources>& zone, ThreadResources& threadResources)
		{
			threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1 });
		}
	};

	Zone<ThreadResources> Zone1(void* context)
	{
		return Zone<ThreadResources>::create<Zone1Functions>(1u, context);
	}
}