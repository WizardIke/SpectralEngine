#include "zone1.h"
#include <D3D12GraphicsEngine.h>
#include <HresultException.h>
#include <TextureManager.h>
#include <MeshManager.h>
#include <Vector2.h>
#include <D3D12Resource.h>
#include <d3d12.h>
#include <Mesh.h>
#include "../Assets.h"
#include "../PipelineStateObjects.h"
#include "../TextureNames.h"
#include "../MeshNames.h"
#include <TemplateFloat.h>
#include <D3D12DescriptorHeap.h>
#include "../Executor.h"
#include "Array.h"
#include <Vector4.h>
#include <ReflectionCamera.h>
#include <Range.h>

#include <Light.h>
#include <PointLight.h>

#include "../Models/BathModel2.h"
#include "../Models/GroundModel2.h"
#include "../Models/WallModel2.h"
#include "../Models/WaterModel2.h"
#include "../Models/IceModel2.h"
#include "../Models/FireModel.h"

namespace
{
	class HDResources
	{
		constexpr static unsigned char numMeshes = 8u;
		constexpr static unsigned char numTextures = 9u;
		constexpr static unsigned char numComponents = 2u + numTextures + numMeshes;
		constexpr static unsigned int numRenderTargetTextures = 1u;
		constexpr static unsigned int numPointLights = 4u;


		static void componentUploaded(void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(requester);
			zone->componentUploaded(executor, numComponents);
		}

		static RenderPass1::Local::RenderToTextureSubPassGroup& getRenderToTextureSubPassGroup(Executor& executor)
		{
			return executor.renderPass.renderToTextureSubPassGroup();
		}
	public:
		Mesh* squareWithPos;
		Mesh* cubeWithPos;
		D3D12Resource perObjectConstantBuffers;
		uint8_t* perObjectConstantBuffersCpuAddress;
		Array<D3D12Resource, numRenderTargetTextures> renderTargetTextures;
		D3D12_RESOURCE_STATES waterRenderTargetTextureState;
		D3D12DescriptorHeap renderTargetTexturesDescriptorHeap;
		unsigned int shaderResourceViews[numRenderTargetTextures];
		Array<ReflectionCamera, numRenderTargetTextures> reflectionCameras;
		Array<RenderPass1::RenderToTextureSubPass*, numRenderTargetTextures> renderTargetTextureSubPasses;
		unsigned int reflectionCameraBackBuffers[numRenderTargetTextures];

		Light light;
		PointLight pointLights[numPointLights];
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;

		GroundModel2 groundModel;
		WallModel2 wallModel;
		BathModel2 bathModel1;
		WaterModel2 waterModel;
		IceModel2 iceModel;
		FireModel<templateFloat(55.0f), templateFloat(2.0f), templateFloat(64.0f)> fireModel1;
		FireModel<templateFloat(53.0f), templateFloat(2.0f), templateFloat(64.0f)> fireModel2;

		//AudioObject3dClass sound3D1;

		HDResources(Executor* const executor, Assets& sharedResources, BaseZone* zone) :
			light({ 0.1f, 0.1f, 0.1f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -0.894427191f, 0.447213595f }),

			perObjectConstantBuffers(sharedResources.graphicsEngine.graphicsDevice, []()
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
		}(), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr),

			renderTargetTexturesDescriptorHeap(sharedResources.graphicsEngine.graphicsDevice, []()
		{
			D3D12_DESCRIPTOR_HEAP_DESC descriptorheapDesc;
			descriptorheapDesc.NumDescriptors = numRenderTargetTextures;
			descriptorheapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //RTVs aren't shader visable
			descriptorheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			descriptorheapDesc.NodeMask = 0u;
			return descriptorheapDesc;
		}())

		{
#ifndef NDEBUG
			renderTargetTexturesDescriptorHeap->SetName(L"Zone 1 Render Target texture descriptor heap");
#endif
			pointLights[0u] = PointLight{ Vector4{ 20.0f, 2.0f, 2.0f, 1.0f }, Vector4{ 52.0f, 5.0f, 76.0f, 1.f } };
			pointLights[1u] = PointLight{ Vector4{ 2.0f, 20.0f, 2.0f, 1.0f }, Vector4{ 76.0f, 5.0f, 76.0f, 1.f } };
			pointLights[2u] = PointLight{ Vector4{ 2.0f, 2.0f, 20.0f, 1.0f },Vector4{ 52.0f, 5.0f, 52.0f, 1.f } };
			pointLights[3u] = PointLight{ Vector4{ 20.0f, 20.0f, 20.0f, 1.0f }, Vector4{ 76.0f, 5.0f, 52.0f, 1.f } };

			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw HresultException(hr);
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();
			uint8_t* cpuConstantBuffer = perObjectConstantBuffersCpuAddress;

			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.VisibleNodeMask = 0u;
			heapProperties.CreationNodeMask = 0u;

			D3D12_RESOURCE_DESC resourcesDesc;
			resourcesDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourcesDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resourcesDesc.Width = sharedResources.window.width();
			resourcesDesc.Height = sharedResources.window.height();
			resourcesDesc.DepthOrArraySize = 1u;
			resourcesDesc.MipLevels = 1u;
			resourcesDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			resourcesDesc.SampleDesc = { 1u, 0u };
			resourcesDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resourcesDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_CLEAR_VALUE clearValue;
			clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			clearValue.Color[0] = 0.0f;
			clearValue.Color[1] = 0.0f;
			clearValue.Color[2] = 0.0f;
			clearValue.Color[3] = 1.0f;

			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				new(&renderTargetTextures[i]) D3D12Resource(sharedResources.graphicsEngine.graphicsDevice, heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, resourcesDesc,
					D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue);

#ifdef _DEBUG
				std::wstring name = L" zone1 render to texture ";
				name += std::to_wstring(i);
				renderTargetTextures[i]->SetName(name.c_str());
#endif // _DEBUG
			}

			D3D12_RENDER_TARGET_VIEW_DESC HDRenderTargetViewDesc;
			HDRenderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			HDRenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			HDRenderTargetViewDesc.Texture2D.MipSlice = 0u;
			HDRenderTargetViewDesc.Texture2D.PlaneSlice = 0u;

			D3D12_SHADER_RESOURCE_VIEW_DESC HDSHaderResourceViewDesc;
			HDSHaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			HDSHaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			HDSHaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			HDSHaderResourceViewDesc.Texture2D.MipLevels = 1u;
			HDSHaderResourceViewDesc.Texture2D.MostDetailedMip = 0u;
			HDSHaderResourceViewDesc.Texture2D.PlaneSlice = 0u;
			HDSHaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			auto shaderResourceViewCpuDescriptorHandle = sharedResources.graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto srvSize = sharedResources.graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;
			auto tempRenderTargetTexturesCpuDescriptorHandle = renderTargetTexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto depthStencilHandle = sharedResources.graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

			D3D12_SHADER_RESOURCE_VIEW_DESC backBufferSrvDesc;
			backBufferSrvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			backBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
			backBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			backBufferSrvDesc.Texture2D.MipLevels = 1u;
			backBufferSrvDesc.Texture2D.MostDetailedMip = 0u;
			backBufferSrvDesc.Texture2D.PlaneSlice = 0u;
			backBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0u;

			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				shaderResourceViews[i] = sharedResources.graphicsEngine.descriptorAllocator.allocate();
				sharedResources.graphicsEngine.graphicsDevice->CreateShaderResourceView(renderTargetTextures[i], &HDSHaderResourceViewDesc, shaderResourceViewCpuDescriptorHandle + srvSize * shaderResourceViews[i]);

				sharedResources.graphicsEngine.graphicsDevice->CreateRenderTargetView(renderTargetTextures[i], &HDRenderTargetViewDesc, tempRenderTargetTexturesCpuDescriptorHandle);
				Transform transform = sharedResources.mainCamera.transform().reflection(waterModel.reflectionHeight());

				reflectionCameraBackBuffers[i] = sharedResources.graphicsEngine.descriptorAllocator.allocate();
				sharedResources.graphicsEngine.graphicsDevice->CreateShaderResourceView(renderTargetTextures[i], &backBufferSrvDesc,
					shaderResourceViewCpuDescriptorHandle + reflectionCameraBackBuffers[i] * sharedResources.graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize);
				uint32_t backBuffers[frameBufferCount];
				for (auto j = 0u; j < frameBufferCount; ++j)
				{
					backBuffers[j] = reflectionCameraBackBuffers[i];
				}

				new(&reflectionCameras[i]) ReflectionCamera(&sharedResources, renderTargetTextures[i], tempRenderTargetTexturesCpuDescriptorHandle, depthStencilHandle,
					sharedResources.window.width(), sharedResources.window.height(), PerObjectConstantBuffersGpuAddress, cpuConstantBuffer, 0.25f * 3.141f,
					transform, backBuffers);

				tempRenderTargetTexturesCpuDescriptorHandle.ptr += sharedResources.graphicsEngine.renderTargetViewDescriptorSize;
			}

			new(&bathModel1) BathModel2(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			new(&groundModel) GroundModel2(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			new(&wallModel) WallModel2(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			new(&waterModel) WaterModel2(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer, shaderResourceViews[0],
				sharedResources.warpTextureDescriptorIndex);

			new(&iceModel) IceModel2(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer, sharedResources.warpTextureDescriptorIndex);

			new(&fireModel1) FireModel<templateFloat(55.0f), templateFloat(2.0f), templateFloat(64.0f)>(PerObjectConstantBuffersGpuAddress,
				cpuConstantBuffer);

			new(&fireModel2) FireModel<templateFloat(53.0f), templateFloat(2.0f), templateFloat(64.0f)>(PerObjectConstantBuffersGpuAddress,
				cpuConstantBuffer);

			TextureManager::loadTexture(executor, sharedResources, TextureNames::ground01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->groundModel.setDiffuseTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });
			TextureManager::loadTexture(executor, sharedResources, TextureNames::wall01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->wallModel.setDiffuseTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });
			TextureManager::loadTexture(executor, sharedResources, TextureNames::marble01, {zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->bathModel1.setDiffuseTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
					componentUploaded(requester, executor, sharedResources);
			}});

			TextureManager::loadTexture(executor, sharedResources, TextureNames::water01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->waterModel.setNormalTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });
			TextureManager::loadTexture(executor, sharedResources, TextureNames::ice01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->iceModel.setDiffuseTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });
			TextureManager::loadTexture(executor, sharedResources, TextureNames::icebump01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->iceModel.setNormalTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });
			TextureManager::loadTexture(executor, sharedResources, TextureNames::firenoise01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->fireModel1.setNormalTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				resources->fireModel2.setNormalTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });
			TextureManager::loadTexture(executor, sharedResources, TextureNames::fire01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->fireModel1.setDiffuseTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				resources->fireModel2.setDiffuseTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });
			TextureManager::loadTexture(executor, sharedResources, TextureNames::firealpha01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->fireModel1.setAlphaTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				resources->fireModel2.setAlphaTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				componentUploaded(requester, executor, sharedResources);
			} });

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::bath, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->bathModel1.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::plane1, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->groundModel.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::wall, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->wallModel.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			MeshManager::loadMeshWithPositionTexture(MeshNames::water, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->waterModel.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			MeshManager::loadMeshWithPositionTexture(MeshNames::cube, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->iceModel.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			MeshManager::loadMeshWithPositionTexture(MeshNames::square, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->fireModel1.mesh = mesh;
				resources->fireModel2.mesh = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			MeshManager::loadMeshWithPosition(MeshNames::aabb, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->cubeWithPos = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			MeshManager::loadMeshWithPosition(MeshNames::squareWithPos, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				auto resources = ((HDResources*)zone->newData);
				resources->squareWithPos = mesh;
				componentUploaded(requester, executor, sharedResources);
			});
			
			executor->updateJobQueue().push(Job(zone, [](void* zone1, BaseExecutor* executor1, SharedResources& sharedResources)
			{
				const auto zone = reinterpret_cast<BaseZone*>(zone1);
				const auto executor = reinterpret_cast<Executor*>(executor1);
				const auto assets = reinterpret_cast<Assets*>(&sharedResources);
				const auto resource = reinterpret_cast<HDResources*>(zone->newData);

				for (auto i = 0u; i < numRenderTargetTextures; ++i)
				{
					auto executors = assets->executors();
					resource->renderTargetTextureSubPasses[i] = &assets->renderPass.renderToTextureSubPassGroup().addSubPass(sharedResources, assets->renderPass,
						executors.map<RenderPass1::Local::RenderToTextureSubPassGroup, getRenderToTextureSubPassGroup>());
					resource->renderTargetTextureSubPasses[i]->addCamera(sharedResources, assets->renderPass, &resource->reflectionCameras[i]);
				}
				componentUploaded(zone, executor, sharedResources);
			}));


			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & 
				~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);
			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(cpuConstantBuffer);
			cpuConstantBuffer += frameBufferCount * pointLightConstantBufferAlignedSize;

			pointLightConstantBufferGpuAddress = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += frameBufferCount * pointLightConstantBufferAlignedSize;
			//sound3D1.Play(DSBPLAY_LOOPING);
		}

		~HDResources() {}

		static void create(void*const zone1, BaseExecutor*const executor1, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			const auto executor = reinterpret_cast<Executor*const >(executor1);
			
			zone->newData = malloc(sizeof(HDResources));
			new(zone->newData) HDResources(executor, (Assets&)sharedResources, zone);
			componentUploaded(zone, executor, sharedResources);
		}

		void update1(BaseExecutor* const executor1, SharedResources& sharedResources)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			const auto assets = reinterpret_cast<Assets*>(&sharedResources);
			const auto frameIndex = assets->graphicsEngine.frameIndex;
			const auto& cameraPos = assets->mainCamera.position();
			const auto frameTime = assets->timer.frameTime();
			waterModel.update(sharedResources);
			fireModel1.update(frameIndex, cameraPos, frameTime);
			fireModel2.update(frameIndex, cameraPos, frameTime);
			for (auto& camera : reflectionCameras)
			{
				camera.update(assets, assets->mainCamera.transform().reflection(waterModel.reflectionHeight()).toMatrix());
			}

			auto rotationMatrix = DirectX::XMMatrixTranslation(-64.0f, -5.0f, -64.0f) * DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), assets->timer.frameTime()) * DirectX::XMMatrixTranslation(64.0f, 5.0f, 64.0f);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
			auto pointLightConstantBuffer = reinterpret_cast<LightConstantBuffer*>(reinterpret_cast<uint8_t*>(pointLightConstantBufferCpuAddress) + frameIndex * pointLightConstantBufferAlignedSize);

			pointLightConstantBuffer->ambientLight = light.ambientLight;
			pointLightConstantBuffer->directionalLight = light.directionalLight;
			pointLightConstantBuffer->lightDirection = light.direction;

			pointLightConstantBuffer->pointLightCount = numPointLights;
			for (auto i = 0u; i < numPointLights; ++i)
			{
				auto temp = DirectX::XMVector4Transform(DirectX::XMVectorSet(pointLights[i].position.x(), pointLights[i].position.y(),
					pointLights[i].position.z(), pointLights[i].position.w()), rotationMatrix);
				DirectX::XMFLOAT4 temp2;
				DirectX::XMStoreFloat4(&temp2, temp);
				pointLights[i].position.x() = temp2.x; pointLights[i].position.y() = temp2.y; pointLights[i].position.z() = temp2.z; pointLights[i].position.w() = temp2.w;
				pointLightConstantBuffer->pointLights[i] = pointLights[i];
			}
		}

		void renderToTexture(BaseExecutor* const executor1, SharedResources& sharedResources)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			const auto frameIndex = sharedResources.graphicsEngine.frameIndex;
			Assets* const assets = (Assets*)&sharedResources;

			auto depthStencilHandle = assets->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			if (waterModel.isInView(assets->mainCamera.frustum()))
			{
				const auto subPass = assets->renderPass.renderToTextureSubPassGroup().subPasses().begin();
				const auto camera = *subPass->cameras().begin();
				const auto& frustum = camera->frustum();
				const auto commandList = executor->renderPass.renderToTextureSubPassGroup().subPasses().begin()->firstCommandList();
				auto warpTextureCpuDescriptorHandle = assets->warpTextureCpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				auto backBufferRenderTargetCpuHandle = camera->getRenderTargetView(assets);

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress + frameIndex * pointLightConstantBufferAlignedSize);

				D3D12_RESOURCE_BARRIER copyStartBarriers[2u];
				copyStartBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				copyStartBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				copyStartBarriers[0u].Transition.pResource = assets->warpTexture;
				copyStartBarriers[0u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				copyStartBarriers[0u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				copyStartBarriers[0u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				copyStartBarriers[1u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				copyStartBarriers[1u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				copyStartBarriers[1u].Transition.pResource = camera->getImage();
				copyStartBarriers[1u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				copyStartBarriers[1u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				copyStartBarriers[1u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				D3D12_RESOURCE_BARRIER copyEndBarriers[2u];
				copyEndBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				copyEndBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				copyEndBarriers[0u].Transition.pResource = assets->warpTexture;
				copyEndBarriers[0u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				copyEndBarriers[0u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				copyEndBarriers[0u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				copyEndBarriers[1u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				copyEndBarriers[1u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				copyEndBarriers[1u].Transition.pResource = copyStartBarriers[1u].Transition.pResource;
				copyEndBarriers[1u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				copyEndBarriers[1u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				copyEndBarriers[1u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				if (wallModel.isInView(frustum))
				{
					commandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);
					commandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsBufferGpu());
					commandList->SetGraphicsRootConstantBufferView(3u, wallModel.psBufferGpu());
					commandList->IASetVertexBuffers(0u, 1u, &wallModel.mesh->vertexBufferView);
					commandList->IASetIndexBuffer(&wallModel.mesh->indexBufferView);
					commandList->DrawIndexedInstanced(wallModel.mesh->indexCount, 1u, 0u, 0, 0u);
				}
				if (iceModel.isInView(frustum))
				{
					commandList->ResourceBarrier(2u, copyStartBarriers);
					commandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
					commandList->SetPipelineState(assets->pipelineStateObjects.copy);
					commandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsAabbGpu());
					commandList->IASetVertexBuffers(0u, 1u, &cubeWithPos->vertexBufferView);
					commandList->IASetIndexBuffer(&cubeWithPos->indexBufferView);
					commandList->DrawIndexedInstanced(cubeWithPos->indexCount, 1u, 0u, 0, 0u);

					commandList->ResourceBarrier(2u, copyEndBarriers);
					commandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
					commandList->SetPipelineState(assets->pipelineStateObjects.glass);
					commandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsConstantBufferGPU(frameIndex));
					commandList->SetGraphicsRootConstantBufferView(3u, iceModel.psConstantBufferGPU());
					commandList->IASetVertexBuffers(0u, 1u, &iceModel.mesh->vertexBufferView);
					commandList->IASetIndexBuffer(&iceModel.mesh->indexBufferView);
					commandList->DrawIndexedInstanced(iceModel.mesh->indexCount, 1u, 0u, 0, 0u);
				}
			}
		}

		void update2(BaseExecutor* const executor1, SharedResources& sharedResources)
		{
			renderToTexture(executor1, sharedResources);

			const auto executor = reinterpret_cast<Executor* const>(executor1);
			Assets* const assets = (Assets*)&sharedResources;
			const auto frameIndex = sharedResources.graphicsEngine.frameIndex;
			const auto opaqueDirectCommandList = executor->renderPass.colorSubPass().opaqueCommandList();
			const auto transparantCommandList = executor->renderPass.colorSubPass().transparentCommandList();
			auto depthStencilHandle = assets->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto backBufferRenderTargetCpuHandle = assets->mainCamera.getRenderTargetView(assets);
			auto warpTextureCpuDescriptorHandle = assets->warpTextureCpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

			D3D12_RESOURCE_BARRIER copyStartBarriers[2u];
			copyStartBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyStartBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			copyStartBarriers[0u].Transition.pResource = assets->warpTexture;
			copyStartBarriers[0u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			copyStartBarriers[0u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			copyStartBarriers[0u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			copyStartBarriers[1u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyStartBarriers[1u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			copyStartBarriers[1u].Transition.pResource = assets->window.getBuffer(frameIndex);
			copyStartBarriers[1u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			copyStartBarriers[1u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			copyStartBarriers[1u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			D3D12_RESOURCE_BARRIER copyEndBarriers[2u];
			copyEndBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyEndBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			copyEndBarriers[0u].Transition.pResource = assets->warpTexture;
			copyEndBarriers[0u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			copyEndBarriers[0u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			copyEndBarriers[0u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			copyEndBarriers[1u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyEndBarriers[1u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			copyEndBarriers[1u].Transition.pResource = copyStartBarriers[1u].Transition.pResource;
			copyEndBarriers[1u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			copyEndBarriers[1u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			copyEndBarriers[1u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			opaqueDirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & ~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);
			opaqueDirectCommandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress + frameIndex * pointLightConstantBufferAlignedSize);
			const auto& frustum = assets->mainCamera.frustum();

			if (groundModel.isInView(frustum))
			{
				opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.pointLight);

				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, groundModel.vsBufferGpu());
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, groundModel.psBufferGpu());
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &groundModel.mesh->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&groundModel.mesh->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(groundModel.mesh->indexCount, 1u, 0u, 0, 0u);
			}

			opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);
			if (wallModel.isInView(frustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsBufferGpu());
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, wallModel.psBufferGpu());
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &wallModel.mesh->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&wallModel.mesh->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(wallModel.mesh->indexCount, 1u, 0u, 0, 0u);
			}

			if (bathModel1.isInView(frustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, bathModel1.vsBufferGpu());
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, bathModel1.psBufferGpu());
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &bathModel1.mesh->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&bathModel1.mesh->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(bathModel1.mesh->indexCount, 1u, 0u, 0, 0u);
			}

			transparantCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			if (waterModel.isInView(frustum))
			{
				transparantCommandList->ResourceBarrier(2u, copyStartBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.copy);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsAabbGpu());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &squareWithPos->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&squareWithPos->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(squareWithPos->indexCount, 1u, 0u, 0, 0u);

				transparantCommandList->ResourceBarrier(2u, copyEndBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.waterWithReflectionTexture);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsConstantBufferGPU(frameIndex));
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, waterModel.psConstantBufferGPU());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &waterModel.mesh->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&waterModel.mesh->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(waterModel.mesh->indexCount, 1u, 0u, 0, 0u);
			}


			if (iceModel.isInView(frustum))
			{
				transparantCommandList->ResourceBarrier(2u, copyStartBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.copy);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsAabbGpu());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &cubeWithPos->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&cubeWithPos->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(cubeWithPos->indexCount, 1u, 0u, 0, 0u);

				transparantCommandList->ResourceBarrier(2u, copyEndBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.glass);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsConstantBufferGPU(frameIndex));
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, iceModel.psConstantBufferGPU());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &iceModel.mesh->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&iceModel.mesh->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(iceModel.mesh->indexCount, 1u, 0u, 0, 0u);
			}
			
			if (fireModel1.isInView(frustum) || fireModel2.isInView(frustum))
			{
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.fire);

				transparantCommandList->IASetVertexBuffers(0u, 1u, &fireModel1.mesh->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&fireModel1.mesh->indexBufferView);

				if (fireModel1.isInView(frustum))
				{
					transparantCommandList->SetGraphicsRootConstantBufferView(2u, fireModel1.vsConstantBufferGPU(frameIndex));
					transparantCommandList->SetGraphicsRootConstantBufferView(3u, fireModel1.psConstantBufferGPU());
					transparantCommandList->DrawIndexedInstanced(fireModel1.mesh->indexCount, 1u, 0u, 0, 0u);
				}

				if (fireModel2.isInView(frustum))
				{
					transparantCommandList->SetGraphicsRootConstantBufferView(2u, fireModel2.vsConstantBufferGPU(frameIndex));
					transparantCommandList->SetGraphicsRootConstantBufferView(3u, fireModel2.psConstantBufferGPU());
					transparantCommandList->DrawIndexedInstanced(fireModel2.mesh->indexCount, 1u, 0u, 0, 0u);
				}
			}
		}

		void destruct(BaseExecutor*const executor, SharedResources& sharedResources)
		{
			auto& textureManager = sharedResources.textureManager;
			auto& meshManager = sharedResources.meshManager;
			auto& graphicsEngine = sharedResources.graphicsEngine;
			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				graphicsEngine.descriptorAllocator.deallocate(shaderResourceViews[i]);

				executor->updateJobQueue().push(Job(renderTargetTextureSubPasses[i], [](void* subPass1, BaseExecutor* executor1, SharedResources& sharedResources)
				{
					const auto subPass = reinterpret_cast<RenderPass1::RenderToTextureSubPass*>(subPass1);
					const auto executor = reinterpret_cast<Executor*>(executor1);
					const auto assets = reinterpret_cast<Assets*>(&sharedResources);
					auto executors = assets->executors();
					assets->renderPass.renderToTextureSubPassGroup().removeSubPass(sharedResources, subPass, 
						executors.map<RenderPass1::Local::RenderToTextureSubPassGroup, getRenderToTextureSubPassGroup>());
				}));

				graphicsEngine.descriptorAllocator.deallocate(reflectionCameraBackBuffers[i]);
			}

			textureManager.unloadTexture(TextureNames::ground01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::wall01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::marble01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::water01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::ice01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::icebump01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::firenoise01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::fire01, graphicsEngine);
			textureManager.unloadTexture(TextureNames::firealpha01, graphicsEngine);

			meshManager.unloadMesh(MeshNames::bath, executor);
			meshManager.unloadMesh(MeshNames::plane1, executor);
			meshManager.unloadMesh(MeshNames::wall, executor);
			meshManager.unloadMesh(MeshNames::water, executor);
			meshManager.unloadMesh(MeshNames::cube, executor);
			meshManager.unloadMesh(MeshNames::square, executor);

			perObjectConstantBuffers->Unmap(0u, nullptr);
		}
	};

	class MDResources
	{
		constexpr static unsigned int numComponents = 3u;
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;


		static void callback(void* requester, BaseExecutor* executor, SharedResources& sharedResources)
		{
			auto zone = reinterpret_cast<BaseZone* const>(requester);
			zone->componentUploaded(executor, numComponents);
		}
	public:
		D3D12Resource perObjectConstantBuffers;
		uint8_t* perObjectConstantBuffersCpuAddress;

		BathModel2 bathModel;
		Light light;


		MDResources(BaseExecutor* const exe, SharedResources& sharedResources, void* zone) :
			light(DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
			perObjectConstantBuffers(sharedResources.graphicsEngine.graphicsDevice, []()
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
			const auto executor = reinterpret_cast<Executor*>(exe);

			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw HresultException(hr);
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();
			auto cpuConstantBuffer = perObjectConstantBuffersCpuAddress;

			new(&bathModel) BathModel2(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			TextureManager::loadTexture(executor, sharedResources, TextureNames::marble01, { zone, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, unsigned int textureID) {
				const auto zone = reinterpret_cast<BaseZone*>(requester);
				auto resources = ((MDResources*)zone->newData);
				resources->bathModel.setDiffuseTexture(textureID, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
				callback(requester, executor, sharedResources);
			} });

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::bath, zone, executor, sharedResources, [](void* requester, BaseExecutor* executor, SharedResources& sharedResources, Mesh* mesh)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(requester);
				const auto resources = ((MDResources*)zone->newData);
				resources->bathModel.mesh = mesh;
				callback(requester, executor, sharedResources);
			});
		}

		void destruct(BaseExecutor*const executor, SharedResources& sharedResources)
		{
			auto& textureManager = sharedResources.textureManager;
			auto& meshManager = sharedResources.meshManager;
			auto& graphicsEngine = sharedResources.graphicsEngine;

			textureManager.unloadTexture(TextureNames::marble01, graphicsEngine);

			meshManager.unloadMesh(MeshNames::bath, executor);

			perObjectConstantBuffers->Unmap(0u, nullptr);
		}

		~MDResources() {}

		void update2(BaseExecutor* const executor1, SharedResources& sharedResources)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			uint32_t frameIndex = sharedResources.graphicsEngine.frameIndex;
			const auto commandList = executor->renderPass.colorSubPass().opaqueCommandList();
			Assets* const assets = (Assets*)&sharedResources;

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (bathModel.isInView(assets->mainCamera.frustum()))
			{
				commandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);

				commandList->SetGraphicsRootConstantBufferView(2u, bathModel.vsBufferGpu());
				commandList->SetGraphicsRootConstantBufferView(3u, bathModel.psBufferGpu());
				commandList->IASetVertexBuffers(0u, 1u, &bathModel.mesh->vertexBufferView);
				commandList->IASetIndexBuffer(&bathModel.mesh->indexBufferView);
				commandList->DrawIndexedInstanced(bathModel.mesh->indexCount, 1u, 0u, 0, 0u);
			}
		}

		void update1(BaseExecutor* const executor, SharedResources& sharedResources) {}

		static void create(void*const zone1, BaseExecutor*const executor, SharedResources& sharedResources)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			
			zone->newData = malloc(sizeof(MDResources));
			new(zone->newData) MDResources(executor, sharedResources, zone);
			callback(zone, executor, sharedResources);
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
		case 1u:
			reinterpret_cast<MDResources*>(zone->currentData)->update1(executor, sharedResources);
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
		case 1u:
			reinterpret_cast<MDResources*>(zone->lastUsedData)->update2(executor, sharedResources);
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
			case 1u:
				sharedResources.backgroundQueue.push(Job(zone, &MDResources::create));
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
			case 1u:
				sharedResources.backgroundQueue.push(Job(zone, [](void*const zone1, BaseExecutor*const executor, SharedResources& sharedResources)
				{
					const auto zone = reinterpret_cast<BaseZone*const>(zone1);
					auto resource = reinterpret_cast<MDResources*>(zone->oldData);
					resource->destruct(executor, sharedResources);
					resource->~MDResources();
					free(resource);
					zone->finishedDeletingOldState(executor);
				}));
				break;
			}
		}

		static void loadConnectedAreas(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources, float distance, Area::VisitedNode* loadedAreas)
		{
			Assets* const assets = (Assets*)&sharedResources;
			assets->areas.cave.load(executor, sharedResources, Vector2{ 0.0f, 91.0f }, distance, loadedAreas);
		}
		static bool changeArea(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
		{
			Assets* const assets = (Assets*)&sharedResources;
			auto& playerPosition = assets->playerPosition.location.position;
			if ((playerPosition.x - 74.0f) * (playerPosition.x - 74.0f) + (playerPosition.y + 30.0f) * (playerPosition.y + 30.0f) + (playerPosition.z - 51.0f) * (playerPosition.z - 51.0f) < 202.0f)
			{
				assets->areas.cave.setAsCurrentArea(executor, sharedResources);
				return true;
			}
			return false;
		}

		static void start(BaseZone* zone, BaseExecutor* const executor, SharedResources& sharedResources)
		{
			executor->updateJobQueue().push(Job(zone, update1));
		}
	};
}

BaseZone Zone1()
{
	return BaseZone::create<Zone1Functions>(2u);
}