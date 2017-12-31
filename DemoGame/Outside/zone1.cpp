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
#include <array>
#include <Vector4.h>
#include <Camera.h>

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


		static void componentUploaded(void* requester, BaseExecutor* executor)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(requester);
			BaseZone::componentUploaded<BaseZone::high, HDResources::numComponents>(zone, executor, ((HDResources*)zone->nextResources)->numComponentsLoaded);
		}

		std::atomic<unsigned char> numComponentsLoaded = 0u;

		static RenderPass1::Local::RenderToTextureSubPassGroup& getRenderToTextureSubPassGroup(Executor& executor)
		{
			return executor.renderPass.renderToTextureSubPassGroup();
		}

		struct AabbMaterialPS
		{
			uint32_t srcTexture;
		};

		constexpr static size_t psAabbBufferSize = (sizeof(AabbMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	public:
		Mesh* meshes[numMeshes];
		D3D12Resource perObjectConstantBuffers;
		std::array<D3D12Resource, numRenderTargetTextures> renderTargetTextures;
		D3D12_RESOURCE_STATES waterRenderTargetTextureState;
		D3D12DescriptorHeap renderTargetTexturesDescriptorHeap;
		unsigned int shaderResourceViews[numRenderTargetTextures];
		std::array<Camera, numRenderTargetTextures> reflectionCameras;
		std::array<RenderPass1::RenderToTextureSubPass*, numRenderTargetTextures> renderTargetTextureSubPasses;

		Light light;
		PointLight pointLights[numPointLights];
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS warpRenderingConstantBuffer;
		unsigned int backBufferSrvs[frameBufferCount];

		GroundModel2 groundModel;
		WallModel2 wallModel;
		BathModel2 bathModel1;
		WaterModel2 waterModel;
		IceModel2 iceModel;
		FireModel<templateFloat(55.0f), templateFloat(2.0f), templateFloat(64.0f)> fireModel1;
		FireModel<templateFloat(53.0f), templateFloat(2.0f), templateFloat(64.0f)> fireModel2;

		//AudioObject3dClass sound3D1;

		HDResources(Executor* const executor, BaseZone* zone) :
			light({ 0.1f, 0.1f, 0.1f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -0.894427191f, 0.447213595f }),

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

			renderTargetTexturesDescriptorHeap(executor->sharedResources->graphicsEngine.graphicsDevice, []()
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

			const auto sharedResources = (Assets*)executor->sharedResources;

			unsigned int ground01 = TextureManager::loadTexture(TextureNames::ground01, zone, executor, componentUploaded);
			unsigned int wall01 = TextureManager::loadTexture(TextureNames::wall01, zone, executor, componentUploaded);
			unsigned int marble01 = TextureManager::loadTexture(TextureNames::marble01, zone, executor, componentUploaded);
			unsigned int water01 = TextureManager::loadTexture(TextureNames::water01, zone, executor, componentUploaded);
			unsigned int ice01 = TextureManager::loadTexture(TextureNames::ice01, zone, executor, componentUploaded);
			unsigned int icebump01 = TextureManager::loadTexture(TextureNames::icebump01, zone, executor, componentUploaded);
			unsigned int firenoise01 = TextureManager::loadTexture(TextureNames::firenoise01, zone, executor, componentUploaded);
			unsigned int fire01 = TextureManager::loadTexture(TextureNames::fire01, zone, executor, componentUploaded);
			unsigned int firealpha01 = TextureManager::loadTexture(TextureNames::firealpha01, zone, executor, componentUploaded);

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::bath, zone, executor, componentUploaded, &meshes[0]);
			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::plane1, zone, executor, componentUploaded, &meshes[1]);
			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::wall, zone, executor, componentUploaded, &meshes[2]);
			MeshManager::loadMeshWithPositionTexture(MeshNames::water, zone, executor, componentUploaded, &meshes[3]);
			MeshManager::loadMeshWithPositionTexture(MeshNames::cube, zone, executor, componentUploaded, &meshes[4]);
			MeshManager::loadMeshWithPositionTexture(MeshNames::square, zone, executor, componentUploaded, &meshes[5]);
			MeshManager::loadMeshWithPosition(MeshNames::aabb, zone, executor, componentUploaded, &meshes[6]);
			MeshManager::loadMeshWithPosition(MeshNames::squareWithPos, zone, executor, componentUploaded, &meshes[7]);

			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.VisibleNodeMask = 0u;
			heapProperties.CreationNodeMask = 0u;

			D3D12_RESOURCE_DESC resourcesDesc;
			resourcesDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourcesDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resourcesDesc.Width = sharedResources->window.width();
			resourcesDesc.Height = sharedResources->window.height();
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
				new(&renderTargetTextures[i]) D3D12Resource(sharedResources->graphicsEngine.graphicsDevice, heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, resourcesDesc,
					D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue);

#ifdef _DEBUG
				std::wstring name = L" zone1 render to texture ";
				name += std::to_wstring(i);
				renderTargetTextures[i]->SetName(name.c_str());
#endif // _DEBUG
			}

			uint8_t* perObjectConstantBuffersCpuAddress;
			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw HresultException(hr);
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();

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

			auto shaderResourceViewCpuDescriptorHandle = sharedResources->graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto srvSize = sharedResources->graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;
			auto tempRenderTargetTexturesCpuDescriptorHandle = renderTargetTexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto depthStencilHandle = sharedResources->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				shaderResourceViews[i] = sharedResources->graphicsEngine.descriptorAllocator.allocate();
				sharedResources->graphicsEngine.graphicsDevice->CreateShaderResourceView(renderTargetTextures[i], &HDSHaderResourceViewDesc, shaderResourceViewCpuDescriptorHandle + srvSize * shaderResourceViews[i]);

				sharedResources->graphicsEngine.graphicsDevice->CreateRenderTargetView(renderTargetTextures[i], &HDRenderTargetViewDesc, tempRenderTargetTexturesCpuDescriptorHandle);
				Location location;
				sharedResources->mainCamera.makeReflectionLocation(location, waterModel.reflectionHeight());
				new(&reflectionCameras[i]) Camera(sharedResources, renderTargetTextures[i], tempRenderTargetTexturesCpuDescriptorHandle, depthStencilHandle,
					sharedResources->window.width(), sharedResources->window.height(), PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, 0.25f * 3.141f,
					location);

				tempRenderTargetTexturesCpuDescriptorHandle.ptr += sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
			}
			
			executor->updateJobQueue().push(Job(zone, [](void* zone1, BaseExecutor* executor1)
			{
				const auto zone = reinterpret_cast<BaseZone*>(zone1);
				const auto executor = reinterpret_cast<Executor*>(executor1);
				const auto assets = reinterpret_cast<Assets*>(executor->sharedResources);
				const auto resource = reinterpret_cast<HDResources*>(zone->nextResources);

				for (auto i = 0u; i < numRenderTargetTextures; ++i)
				{
					auto executors = assets->executors();
					resource->renderTargetTextureSubPasses[i] = &assets->renderPass.renderToTextureSubPassGroup().addSubPass(executor, assets->renderPass,
						executors.map<RenderPass1::Local::RenderToTextureSubPassGroup, getRenderToTextureSubPassGroup>());
					resource->renderTargetTextureSubPasses[i]->addCamera(executor, assets->renderPass, &resource->reflectionCameras[i]);
				}
				componentUploaded(zone, executor);
			}));


			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & 
				~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);
			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(perObjectConstantBuffersCpuAddress);
			perObjectConstantBuffersCpuAddress += frameBufferCount * pointLightConstantBufferAlignedSize;

			pointLightConstantBufferGpuAddress = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += frameBufferCount * pointLightConstantBufferAlignedSize;

			warpRenderingConstantBuffer = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += psAabbBufferSize * frameBufferCount;

			D3D12_SHADER_RESOURCE_VIEW_DESC backBufferSrvDesc;
			backBufferSrvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			backBufferSrvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
			backBufferSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			backBufferSrvDesc.Texture2D.MipLevels = 1u;
			backBufferSrvDesc.Texture2D.MostDetailedMip = 0u;
			backBufferSrvDesc.Texture2D.PlaneSlice = 0u;
			backBufferSrvDesc.Texture2D.ResourceMinLODClamp = 0u;
			for (auto i = 0u; i < frameBufferCount; ++i)
			{
				backBufferSrvs[i] = sharedResources->graphicsEngine.descriptorAllocator.allocate();
				sharedResources->graphicsEngine.graphicsDevice->CreateShaderResourceView(sharedResources->window.getBuffer(i), &backBufferSrvDesc,
					shaderResourceViewCpuDescriptorHandle + backBufferSrvs[i] * sharedResources->graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize);
				auto psAabbBuffer = reinterpret_cast<AabbMaterialPS*>(perObjectConstantBuffersCpuAddress);
				psAabbBuffer->srcTexture = backBufferSrvs[i];
				perObjectConstantBuffersCpuAddress += psAabbBufferSize;
			}
			

			new(&groundModel) GroundModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, ground01);

			new(&wallModel) WallModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, wall01);

			new(&bathModel1) BathModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, marble01);

			new(&waterModel) WaterModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, shaderResourceViews[0],
				sharedResources->warpTextureDescriptorIndex, water01);

			new(&iceModel) IceModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, sharedResources->warpTextureDescriptorIndex, ice01, icebump01);

			new(&fireModel1) FireModel<templateFloat(55.0f), templateFloat(2.0f), templateFloat(64.0f)>(PerObjectConstantBuffersGpuAddress,
				perObjectConstantBuffersCpuAddress, firenoise01, fire01, firealpha01);

			new(&fireModel2) FireModel<templateFloat(53.0f), templateFloat(2.0f), templateFloat(64.0f)>(PerObjectConstantBuffersGpuAddress,
				perObjectConstantBuffersCpuAddress, firenoise01, fire01, firealpha01);

			//sound3D1.Play(DSBPLAY_LOOPING);
		}

		~HDResources() {}

		static void create(void*const zone1, BaseExecutor*const executor1)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			const auto executor = reinterpret_cast<Executor*const >(executor1);
			
			zone->nextResources = malloc(sizeof(HDResources));
			new(zone->nextResources) HDResources(executor, zone);
			componentUploaded(zone, executor);
		}

		void update1(BaseExecutor* const executor1)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			const auto assets = reinterpret_cast<Assets*>(executor->sharedResources);
			const auto frameIndex = assets->graphicsEngine.frameIndex;
			const auto& cameraPos = assets->mainCamera.position();
			const auto frameTime = assets->timer.frameTime;
			waterModel.update(executor);
			fireModel1.update(frameIndex, cameraPos, frameTime);
			fireModel2.update(frameIndex, cameraPos, frameTime);
			for (auto& camera : reflectionCameras)
			{
				Location location;
				assets->mainCamera.makeReflectionLocation(location, waterModel.reflectionHeight());
				camera.update(assets, Camera::locationToMatrix(location));
			}

			auto rotationMatrix = DirectX::XMMatrixTranslation(-64.0f, -5.0f, -64.0f) * DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), assets->timer.frameTime) * DirectX::XMMatrixTranslation(64.0f, 5.0f, 64.0f);

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

		void renderToTexture(BaseExecutor* const executor1)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			const auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			Assets* const assets = (Assets*)executor->sharedResources;

			auto depthStencilHandle = assets->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			if (waterModel.isInView(assets->mainCamera.frustum()))
			{
				const auto& frustum = (*assets->renderPass.renderToTextureSubPassGroup().subPasses().begin()->cameras().begin())->frustum();
				const auto commandList = executor->renderPass.renderToTextureSubPassGroup().subPasses().begin()->firstCommandList();

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress + frameIndex * pointLightConstantBufferAlignedSize);

				if (wallModel.isInView(frustum))
				{
					commandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);
					commandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsPerObjectCBVGpuAddress);
					commandList->SetGraphicsRootConstantBufferView(3u, wallModel.psPerObjectCBVGpuAddress);
					commandList->IASetVertexBuffers(0u, 1u, &meshes[wallModel.meshIndex]->vertexBufferView);
					commandList->IASetIndexBuffer(&meshes[wallModel.meshIndex]->indexBufferView);
					commandList->DrawIndexedInstanced(meshes[wallModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
				}
				if (iceModel.isInView(frustum))
				{
					commandList->SetPipelineState(assets->pipelineStateObjects.basic);
					commandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsConstantBufferGPU(frameIndex));
					commandList->SetGraphicsRootConstantBufferView(3u, iceModel.psConstantBufferGPU());
					commandList->IASetVertexBuffers(0u, 1u, &meshes[iceModel.meshIndex]->vertexBufferView);
					commandList->IASetIndexBuffer(&meshes[iceModel.meshIndex]->indexBufferView);
					commandList->DrawIndexedInstanced(meshes[iceModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
				}
			}
		}

		void update2(BaseExecutor* const executor1)
		{
			renderToTexture(executor1);

			const auto executor = reinterpret_cast<Executor* const>(executor1);
			Assets* const assets = (Assets*)executor->sharedResources;
			const auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			const auto opaqueDirectCommandList = executor->renderPass.colorSubPass().opaqueCommandList();
			const auto transparantCommandList = executor->renderPass.colorSubPass().transparentCommandList();
			auto depthStencilHandle = assets->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto backBufferRenderTargetCpuHandle = assets->mainCamera.getRenderTargetView();
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

				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, groundModel.vsPerObjectCBVGpuAddress);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, groundModel.psPerObjectCBVGpuAddress);
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[groundModel.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[groundModel.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[groundModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);
			if (wallModel.isInView(frustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsPerObjectCBVGpuAddress);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, wallModel.psPerObjectCBVGpuAddress);
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[wallModel.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[wallModel.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[wallModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			if (bathModel1.isInView(frustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, bathModel1.vsPerObjectCBVGpuAddress);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, bathModel1.psPerObjectCBVGpuAddress);
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[bathModel1.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[bathModel1.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[bathModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			transparantCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			if (waterModel.isInView(frustum))
			{
				transparantCommandList->ResourceBarrier(2u, copyStartBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.copy);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsAabbGpu());
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, warpRenderingConstantBuffer + frameIndex * psAabbBufferSize);
				transparantCommandList->IASetVertexBuffers(0u, 1u, &meshes[7]->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&meshes[7]->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(meshes[7]->indexCount, 1u, 0u, 0, 0u);

				transparantCommandList->ResourceBarrier(2u, copyEndBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.waterWithReflectionTexture);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsConstantBufferGPU(frameIndex));
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, waterModel.psConstantBufferGPU());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &meshes[waterModel.meshIndex]->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&meshes[waterModel.meshIndex]->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(meshes[waterModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}


			if (iceModel.isInView(frustum))
			{
				transparantCommandList->ResourceBarrier(2u, copyStartBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.copy);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsAabbGpu());
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, warpRenderingConstantBuffer + frameIndex * psAabbBufferSize);
				transparantCommandList->IASetVertexBuffers(0u, 1u, &meshes[6]->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&meshes[6]->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(meshes[6]->indexCount, 1u, 0u, 0, 0u);

				transparantCommandList->ResourceBarrier(2u, copyEndBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.glass);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsConstantBufferGPU(frameIndex));
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, iceModel.psConstantBufferGPU());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &meshes[iceModel.meshIndex]->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&meshes[iceModel.meshIndex]->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(meshes[iceModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			if (fireModel1.isInView(frustum) || fireModel2.isInView(frustum))
			{
				transparantCommandList->SetPipelineState(assets->pipelineStateObjects.fire);

				transparantCommandList->IASetVertexBuffers(0u, 1u, &meshes[fireModel1.meshIndex]->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&meshes[fireModel1.meshIndex]->indexBufferView);

				if (fireModel1.isInView(frustum))
				{
					transparantCommandList->SetGraphicsRootConstantBufferView(2u, fireModel1.vsConstantBufferGPU(frameIndex));
					transparantCommandList->SetGraphicsRootConstantBufferView(3u, fireModel1.psConstantBufferGPU());
					transparantCommandList->DrawIndexedInstanced(meshes[fireModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
				}

				if (fireModel2.isInView(frustum))
				{
					transparantCommandList->SetGraphicsRootConstantBufferView(2u, fireModel2.vsConstantBufferGPU(frameIndex));
					transparantCommandList->SetGraphicsRootConstantBufferView(3u, fireModel2.psConstantBufferGPU());
					transparantCommandList->DrawIndexedInstanced(meshes[fireModel2.meshIndex]->indexCount, 1u, 0u, 0, 0u);
				}
			}
		}

		void destruct(BaseExecutor*const executor)
		{
			const auto sharedResources = executor->sharedResources;
			auto& textureManager = sharedResources->textureManager;
			auto& meshManager = sharedResources->meshManager;
			auto& graphicsEngine = sharedResources->graphicsEngine;
			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				graphicsEngine.descriptorAllocator.deallocate(shaderResourceViews[i]);

				executor->updateJobQueue().push(Job(renderTargetTextureSubPasses[i], [](void* subPass1, BaseExecutor* executor1)
				{
					const auto subPass = reinterpret_cast<RenderPass1::RenderToTextureSubPass*>(subPass1);
					const auto executor = reinterpret_cast<Executor*>(executor1);
					const auto assets = reinterpret_cast<Assets*>(executor->sharedResources);
					auto executors = assets->executors();
					assets->renderPass.renderToTextureSubPassGroup().removeSubPass(executor, subPass, 
						executors.map<RenderPass1::Local::RenderToTextureSubPassGroup, getRenderToTextureSubPassGroup>());
				}));
			}

			for (auto i = 0u; i < frameBufferCount; ++i)
			{
				graphicsEngine.descriptorAllocator.deallocate(backBufferSrvs[i]);
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
		}
	};

	class MDResources
	{
		constexpr static unsigned int numComponents = 3u;
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;


		static void callback(void* requester, BaseExecutor* executor)
		{
			auto zone = reinterpret_cast<BaseZone* const>(requester);
			BaseZone::componentUploaded<BaseZone::medium, MDResources::numComponents>(zone, executor, ((MDResources*)zone->nextResources)->numComponentsLoaded);
		}
	public:
		std::atomic<unsigned char> numComponentsLoaded = 0u;

		Mesh* meshes[numMeshes];
		D3D12Resource perObjectConstantBuffers;

		BathModel2 bathModel;
		Light light;


		MDResources(BaseExecutor* const exe, void* zone) :
			light(DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
			perObjectConstantBuffers(exe->sharedResources->graphicsEngine.graphicsDevice, []()
		{
			D3D12_HEAP_PROPERTIES heapProperties;
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.CreationNodeMask = 1u;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.VisibleNodeMask = 1u;
			// heapProperties = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
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
			const auto executor = reinterpret_cast<Executor*>(exe);
			unsigned int marble01 = TextureManager::loadTexture(TextureNames::marble01, zone, executor, callback);

			MeshManager::loadMeshWithPositionTextureNormal(MeshNames::bath, zone, executor, callback, &meshes[0]);

			uint8_t* perObjectConstantBuffersCpuAddress;
			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw HresultException(hr);
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();

			new(&bathModel) BathModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, marble01);
		}

		void destruct(BaseExecutor*const executor)
		{
			const auto sharedResources = executor->sharedResources;
			auto& textureManager = sharedResources->textureManager;
			auto& meshManager = sharedResources->meshManager;
			auto& graphicsEngine = sharedResources->graphicsEngine;

			textureManager.unloadTexture(TextureNames::marble01, graphicsEngine);

			meshManager.unloadMesh(MeshNames::bath, executor);
		}

		~MDResources() {}

		void update2(BaseExecutor* const executor1)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			uint32_t frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			const auto commandList = executor->renderPass.colorSubPass().opaqueCommandList();
			Assets* const assets = (Assets*)executor->sharedResources;

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (bathModel.isInView(assets->mainCamera.frustum()))
			{
				commandList->SetPipelineState(assets->pipelineStateObjects.directionalLight);

				commandList->SetGraphicsRootConstantBufferView(2u, bathModel.vsPerObjectCBVGpuAddress);
				commandList->SetGraphicsRootConstantBufferView(3u, bathModel.psPerObjectCBVGpuAddress);
				commandList->IASetVertexBuffers(0u, 1u, &meshes[bathModel.meshIndex]->vertexBufferView);
				commandList->IASetIndexBuffer(&meshes[bathModel.meshIndex]->indexBufferView);
				commandList->DrawIndexedInstanced(meshes[bathModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}
		}

		void update1(BaseExecutor* const executor) {}

		static void staticJobLoad(void*const zone1, BaseExecutor*const executor)
		{
			const auto zone = reinterpret_cast<BaseZone*const>(zone1);
			
			zone->nextResources = malloc(sizeof(MDResources));
			new(zone->nextResources) MDResources(executor, zone);
			callback(zone, executor);
		}
	};

	static void restart(BaseZone* const zone, BaseExecutor* const executor)
	{
		auto oldLevelOfDetail = zone->levelOfDetail.load(std::memory_order::memory_order_seq_cst);
		if (oldLevelOfDetail == 0u)
		{
			executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(zone1);
				reinterpret_cast<HDResources*>(zone->currentResources)->update1(executor);
				executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
				{
					const auto zone = reinterpret_cast<BaseZone* const>(zone1);
					reinterpret_cast<HDResources*>(zone->currentResources)->update2(executor);
					restart(zone, executor);
				}));
			}));
		}
		else if (oldLevelOfDetail == 1u)
		{
			executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(zone1);
				reinterpret_cast<MDResources*>(zone->currentResources)->update1(executor);
				executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
				{
					const auto zone = reinterpret_cast<BaseZone* const>(zone1);
					reinterpret_cast<MDResources*>(zone->currentResources)->update2(executor);
					restart(zone, executor);
				}));
			}));
		}
		else if (oldLevelOfDetail == 2u)
		{
			executor->updateJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone* const>(zone1);
				executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
				{
					const auto zone = reinterpret_cast<BaseZone* const>(zone1);
					restart(zone, executor);
				}));
			}));
		}
	}

	static void update1(BaseZone* const zone, BaseExecutor* const executor)
	{
		restart(zone, executor);
	}

	static void update2(BaseZone* const zone, BaseExecutor* const executor)
	{
		executor->renderJobQueue().push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(zone1);
			restart(zone, executor);
		}));
	}

	struct Zone1Functions
	{
		static void loadHighDetailJobs(BaseZone* zone, BaseExecutor* const executor)
		{
			executor->sharedResources->backgroundQueue.push(Job(zone, &HDResources::create));
		}
		static void loadMediumDetailJobs(BaseZone* zone, BaseExecutor* const executor)
		{
			executor->sharedResources->backgroundQueue.push(Job(zone, &MDResources::staticJobLoad));
		}
		static void loadLowDetailJobs(BaseZone* zone, BaseExecutor* const executor)
		{
			zone->lastComponentLoaded<BaseZone::low>(executor);
		}

		static void unloadHighDetailJobs(BaseZone* zone, BaseExecutor* const executor)
		{
			executor->sharedResources->backgroundQueue.push(Job(zone, [](void*const highDetailResource, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone*const>(highDetailResource);
				auto resource = reinterpret_cast<HDResources*>(zone->currentResources);
				resource->destruct(executor);
				resource->~HDResources();
				free(zone->currentResources);
				zone->currentResources = nullptr;
				zone->lastComponentUnloaded<BaseZone::high>(executor);
			}));
		}
		static void unloadMediumDetailJobs(BaseZone* zone, BaseExecutor* const executor)
		{
			executor->sharedResources->backgroundQueue.push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone*const>(zone1);
				auto resource = reinterpret_cast<MDResources*>(zone->currentResources);
				resource->~MDResources();
				free(zone->currentResources);
				zone->currentResources = nullptr;
				zone->lastComponentUnloaded<BaseZone::medium>(executor);
			}));
		}
		static void unloadLowDetailJobs(BaseZone* zone, BaseExecutor* const executor)
		{
			executor->sharedResources->backgroundQueue.push(Job(zone, [](void*const zone1, BaseExecutor*const executor)
			{
				const auto zone = reinterpret_cast<BaseZone*const>(zone1);
				zone->lastComponentUnloaded<BaseZone::low>(executor);
			}));
		}

		static void loadConnectedAreas(BaseZone* zone, BaseExecutor* const executor, float distanceSquared, Area::VisitedNode* loadedAreas)
		{
			Assets* const assets = (Assets*)executor->sharedResources;
			assets->areas.cave.load(executor, Vector2{ 0.0f, 91.0f }, std::sqrt(distanceSquared), loadedAreas);
		}
		static bool changeArea(BaseZone* zone, BaseExecutor* const executor)
		{
			Assets* const assets = (Assets*)executor->sharedResources;
			auto& playerPosition = assets->playerPosition.location.position;
			if ((playerPosition.x - 74.0f) * (playerPosition.x - 74.0f) + (playerPosition.y + 30.0f) * (playerPosition.y + 30.0f) + (playerPosition.z - 51.0f) * (playerPosition.z - 51.0f) < 202.0f)
			{
				assets->areas.cave.setAsCurrentArea(executor);
				return true;
			}

			return false;
		}

		static void start(BaseZone* zone, BaseExecutor* const executor)
		{
			const auto sharedResources = executor->sharedResources;
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
}

BaseZone Zone1()
{
	return BaseZone::create<Zone1Functions>();
}