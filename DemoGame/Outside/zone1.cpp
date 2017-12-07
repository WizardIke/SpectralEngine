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
		constexpr static unsigned char numMeshes = 6u;
		constexpr static unsigned char numTextures = 9u;
		constexpr static unsigned char numComponents = 2u + numTextures + numMeshes;
		constexpr static unsigned int numRenderTargetTextures = 3u;
		constexpr static unsigned int numPointLights = 4u;


		static void componentUploaded(void* requester, BaseExecutor* executor)
		{
			const auto zone = reinterpret_cast<BaseZone* const>(requester);
			BaseZone::componentUploaded<BaseZone::high, HDResources::numComponents>(zone, executor, ((HDResources*)zone->nextResources)->numComponentsLoaded);
		}
	public:
		std::atomic<unsigned char> numComponentsLoaded = 0u;

		Mesh* meshes[numMeshes];
		D3D12Resource perObjectConstantBuffers;
		std::array<D3D12Resource, numRenderTargetTextures> renderTargetTextures;
		D3D12_RESOURCE_STATES waterRenderTargetTextureState;
		D3D12DescriptorHeap renderTargetTexturesDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetTexturesCpuDescriptorHandle;
		unsigned int shaderResourceViews[numRenderTargetTextures];

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

		HDResources(Executor* const executor, BaseZone* zone) :
			light({ 0.1f, 0.1f, 0.1f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -0.894427191f, 0.447213595f }),

			//pointLights{ PointLight{Vector4{20.0f, 2.0f, 2.0f, 1.0f}, Vector4{52.0f, 5.0f, 76.0f, 1.f}},
				//PointLight{ Vector4{2.0f, 20.0f, 2.0f, 1.0f}, Vector4{76.0f, 5.0f, 76.0f, 1.f}},
				//PointLight{ Vector4{2.0f, 2.0f, 20.0f, 1.0f},Vector4{52.0f, 5.0f, 52.0f, 1.f}},
				//PointLight{ Vector4{20.0f, 20.0f, 20.0f, 1.0f}, Vector4{76.0f, 5.0f, 52.0f, 1.f}} },

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
			const auto sharedResources = executor->sharedResources;

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
			clearValue.Color[3] = 0.0f;

			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				new(&renderTargetTextures[i]) D3D12Resource(sharedResources->graphicsEngine.graphicsDevice, heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, resourcesDesc,
					D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue);

#ifdef _DEBUG
				std::wstring name = L" zone1 render to texture, texture ";
				name += std::to_wstring(i);
				renderTargetTextures[i]->SetName(name.c_str());
#endif // _DEBUG


				
			}

			executor->renderJobQueue().push(Job(zone, [](void* zone1, BaseExecutor* executor1)
			{
				const auto zone = reinterpret_cast<BaseZone*>(zone1);
				const auto resource = reinterpret_cast<HDResources*>(zone->nextResources);
				const auto executor = reinterpret_cast<Executor*>(executor1);
				D3D12_RESOURCE_BARRIER renderTargetTextureBariers[numRenderTargetTextures];
				for (auto i = 0u; i < numRenderTargetTextures; ++i)
				{
					renderTargetTextureBariers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					renderTargetTextureBariers[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
					renderTargetTextureBariers[i].Transition.pResource = resource->renderTargetTextures[i];
					renderTargetTextureBariers[i].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					renderTargetTextureBariers[i].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
					renderTargetTextureBariers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				}
				executor->renderPass.colorSubPass().opaqueCommandList()->ResourceBarrier(numRenderTargetTextures, renderTargetTextureBariers);
				componentUploaded(zone, executor);
			}));

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

			auto tempRenderTargetTexturesCpuDescriptorHandle = renderTargetTexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			renderTargetTexturesCpuDescriptorHandle = tempRenderTargetTexturesCpuDescriptorHandle;

			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				shaderResourceViews[i] = sharedResources->graphicsEngine.descriptorAllocator.allocate();
			}

			auto tempshaderResourceViewCpuDescriptorHandle = sharedResources->graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto srvSize = sharedResources->graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;

			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				auto temp = tempshaderResourceViewCpuDescriptorHandle;
				temp.ptr += srvSize * shaderResourceViews[i];
				sharedResources->graphicsEngine.graphicsDevice->CreateShaderResourceView(renderTargetTextures[i], &HDSHaderResourceViewDesc, temp);

				sharedResources->graphicsEngine.graphicsDevice->CreateRenderTargetView(renderTargetTextures[i], &HDRenderTargetViewDesc, tempRenderTargetTexturesCpuDescriptorHandle);
				tempRenderTargetTexturesCpuDescriptorHandle.ptr += sharedResources->graphicsEngine.renderTargetViewDescriptorSize;
			}

			uint8_t* perObjectConstantBuffersCpuAddress;
			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw HresultException(hr);
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & ~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);


			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(perObjectConstantBuffersCpuAddress);
			perObjectConstantBuffersCpuAddress += frameBufferCount * pointLightConstantBufferAlignedSize;

			pointLightConstantBufferGpuAddress = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += frameBufferCount * pointLightConstantBufferAlignedSize;

			new(&groundModel) GroundModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, ground01);

			new(&wallModel) WallModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, wall01);

			new(&bathModel1) BathModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, marble01);

			new(&waterModel) WaterModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, shaderResourceViews[0], shaderResourceViews[1], water01);

			new(&iceModel) IceModel2(PerObjectConstantBuffersGpuAddress, perObjectConstantBuffersCpuAddress, shaderResourceViews[2u], ice01, icebump01);

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

		void update2(BaseExecutor* const executor1)
		{
			renderToTexture(executor1);

			const auto executor = reinterpret_cast<Executor* const>(executor1);
			const auto frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			const auto opaqueDirectCommandList = executor->renderPass.colorSubPass().opaqueCommandList();
			Assets* const assets = (Assets*)executor->sharedResources;


			D3D12_RESOURCE_BARRIER beforeTransBarrier[numRenderTargetTextures];
			beforeTransBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			beforeTransBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			beforeTransBarrier[0].Transition.pResource = renderTargetTextures[2];
			beforeTransBarrier[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			beforeTransBarrier[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			beforeTransBarrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			unsigned int beforeBarrierCount = 1u;
			if (waterRenderTargetTextureState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				beforeTransBarrier[beforeBarrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				beforeTransBarrier[beforeBarrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
				beforeTransBarrier[beforeBarrierCount].Transition.pResource = renderTargetTextures[0];
				beforeTransBarrier[beforeBarrierCount].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				beforeTransBarrier[beforeBarrierCount].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				beforeTransBarrier[beforeBarrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				++beforeBarrierCount;

				beforeTransBarrier[beforeBarrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				beforeTransBarrier[beforeBarrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
				beforeTransBarrier[beforeBarrierCount].Transition.pResource = renderTargetTextures[1];
				beforeTransBarrier[beforeBarrierCount].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				beforeTransBarrier[beforeBarrierCount].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				beforeTransBarrier[beforeBarrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				++beforeBarrierCount;
			}

			D3D12_RESOURCE_BARRIER afterTransBarrier[numRenderTargetTextures];
			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				afterTransBarrier[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				afterTransBarrier[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
				afterTransBarrier[i].Transition.pResource = renderTargetTextures[i];
				afterTransBarrier[i].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				afterTransBarrier[i].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				afterTransBarrier[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}

			opaqueDirectCommandList->ResourceBarrier(beforeBarrierCount, beforeTransBarrier);
			opaqueDirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & ~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);
			opaqueDirectCommandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress + frameIndex * pointLightConstantBufferAlignedSize);

			if (groundModel.isInView(assets->mainFrustum))
			{
				opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.pointLightPSO1);

				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, groundModel.vsPerObjectCBVGpuAddress);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, groundModel.psPerObjectCBVGpuAddress);
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[groundModel.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[groundModel.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[groundModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.lightPSO1);
			if (wallModel.isInView(assets->mainFrustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsPerObjectCBVGpuAddress);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, wallModel.psPerObjectCBVGpuAddress);
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[wallModel.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[wallModel.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[wallModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			if (bathModel1.isInView(assets->mainFrustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, bathModel1.vsPerObjectCBVGpuAddress);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, bathModel1.psPerObjectCBVGpuAddress);
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[bathModel1.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[bathModel1.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[bathModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			if (waterModel.isInView(assets->mainFrustum))
			{
				opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.waterPSO1);

				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsPerObjectCBVGpuAddresses[frameIndex]);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, waterModel.psPerObjectCBVGpuAddress);
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[waterModel.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[waterModel.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[waterModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}


			if (iceModel.isInView(assets->mainFrustum))
			{
				opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.glassPSO1);

				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsPerObjectCBVGpuAddress);
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, iceModel.psPerObjectCBVGpuAddress);

				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[iceModel.meshIndex]->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&meshes[iceModel.meshIndex]->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(meshes[iceModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
			}

			const auto transparentDirectCommandList = executor->renderPass.colorSubPass().transparentCommandList();
			transparentDirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			if (fireModel1.isInView(assets->mainFrustum) || fireModel2.isInView(assets->mainFrustum))
			{
				transparentDirectCommandList->SetPipelineState(assets->pipelineStateObjects.firePSO1);

				transparentDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[fireModel1.meshIndex]->vertexBufferView);
				transparentDirectCommandList->IASetIndexBuffer(&meshes[fireModel1.meshIndex]->indexBufferView);

				if (fireModel1.isInView(assets->mainFrustum))
				{
					transparentDirectCommandList->SetGraphicsRootConstantBufferView(2u, fireModel1.vsConstantBufferGPU(frameIndex));
					transparentDirectCommandList->SetGraphicsRootConstantBufferView(3u, fireModel1.psConstantBufferGPU());
					transparentDirectCommandList->DrawIndexedInstanced(meshes[fireModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
				}

				if (fireModel2.isInView(assets->mainFrustum))
				{
					transparentDirectCommandList->SetGraphicsRootConstantBufferView(2u, fireModel2.vsConstantBufferGPU(frameIndex));
					transparentDirectCommandList->SetGraphicsRootConstantBufferView(3u, fireModel2.psConstantBufferGPU());
					transparentDirectCommandList->DrawIndexedInstanced(meshes[fireModel2.meshIndex]->indexCount, 1u, 0u, 0, 0u);
				}
			}

			opaqueDirectCommandList->ResourceBarrier(numRenderTargetTextures, afterTransBarrier);
		}

		void destruct(BaseExecutor*const executor)
		{
			const auto sharedResources = executor->sharedResources;
			auto& textureManager = sharedResources->textureManager;
			auto& meshManager = sharedResources->meshManager;
			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				sharedResources->graphicsEngine.descriptorAllocator.deallocate(shaderResourceViews[i]);
			}

			textureManager.unloadTexture(TextureNames::ground01, executor);
			textureManager.unloadTexture(TextureNames::wall01, executor);
			textureManager.unloadTexture(TextureNames::marble01, executor);
			textureManager.unloadTexture(TextureNames::water01, executor);
			textureManager.unloadTexture(TextureNames::ice01, executor);
			textureManager.unloadTexture(TextureNames::icebump01, executor);
			textureManager.unloadTexture(TextureNames::firenoise01, executor);
			textureManager.unloadTexture(TextureNames::fire01, executor);
			textureManager.unloadTexture(TextureNames::firealpha01, executor);

			meshManager.unloadMesh(MeshNames::bath, executor);
			meshManager.unloadMesh(MeshNames::plane1, executor);
			meshManager.unloadMesh(MeshNames::wall, executor);
			meshManager.unloadMesh(MeshNames::water, executor);
			meshManager.unloadMesh(MeshNames::cube, executor);
			meshManager.unloadMesh(MeshNames::square, executor);
		}


		void renderToTexture(BaseExecutor* const executor1)
		{
			const auto executor = reinterpret_cast<Executor* const>(executor1);
			uint32_t frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
			ID3D12GraphicsCommandList* opaqueDirectCommandList = executor->renderPass.colorSubPass().opaqueCommandList();
			Assets* const assets = (Assets*)executor->sharedResources;
			uint32_t renderCount = 0u;

			constexpr float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			D3D12_RESOURCE_BARRIER BeforeTransBarrier[numRenderTargetTextures];
			for (auto i = 0u; i < numRenderTargetTextures; ++i)
			{
				BeforeTransBarrier[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				BeforeTransBarrier[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
				BeforeTransBarrier[i].Transition.pResource = renderTargetTextures[i];
				BeforeTransBarrier[i].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				BeforeTransBarrier[i].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				BeforeTransBarrier[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}

			D3D12_RESOURCE_BARRIER afterTransBarriers[numRenderTargetTextures];
			afterTransBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			afterTransBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
			afterTransBarriers[0].Transition.pResource = renderTargetTextures[2];
			afterTransBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			afterTransBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			afterTransBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			unsigned int afterBarrierCount = 1u;


			opaqueDirectCommandList->ResourceBarrier(numRenderTargetTextures, BeforeTransBarrier);
			opaqueDirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
			opaqueDirectCommandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress + frameIndex * pointLightConstantBufferAlignedSize);

			auto depthStencilHandle = assets->graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			if (waterModel.isInView(assets->mainFrustum))
			{
				auto tempRenderTargetTexturesCpuDescriptorHandle = renderTargetTexturesCpuDescriptorHandle;
				tempRenderTargetTexturesCpuDescriptorHandle.ptr += 1u * assets->graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;
				opaqueDirectCommandList->OMSetRenderTargets(1u, &tempRenderTargetTexturesCpuDescriptorHandle, TRUE, &depthStencilHandle);
				opaqueDirectCommandList->ClearRenderTargetView(tempRenderTargetTexturesCpuDescriptorHandle, clearColor, 0u, nullptr);
				opaqueDirectCommandList->ClearDepthStencilView(depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u, 0u, nullptr);

				opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.lightPSO1);
				if (bathModel1.isInView(assets->mainFrustum))
				{
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, bathModel1.vsPerObjectCBVGpuAddress);
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, bathModel1.psPerObjectCBVGpuAddress);
					opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[bathModel1.meshIndex]->vertexBufferView);
					opaqueDirectCommandList->IASetIndexBuffer(&meshes[bathModel1.meshIndex]->indexBufferView);
					opaqueDirectCommandList->DrawIndexedInstanced(meshes[bathModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
					++renderCount;
				}

				Frustum renderToTextureFrustum;
				renderToTextureFrustum.update(assets->mainCamera.projectionMatrix, waterModel.reflectionMatrix, assets->mainCamera.screenNear, assets->mainCamera.screenDepth);

				tempRenderTargetTexturesCpuDescriptorHandle = renderTargetTexturesCpuDescriptorHandle;
				opaqueDirectCommandList->OMSetRenderTargets(1u, &tempRenderTargetTexturesCpuDescriptorHandle, TRUE, &depthStencilHandle);
				opaqueDirectCommandList->ClearRenderTargetView(tempRenderTargetTexturesCpuDescriptorHandle, clearColor, 0u, nullptr);
				opaqueDirectCommandList->ClearDepthStencilView(depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

				if (wallModel.isInView(renderToTextureFrustum))
				{
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsPerObjectCBVGpuAddress);
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, wallModel.psPerObjectCBVGpuAddress);
					opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[wallModel.meshIndex]->vertexBufferView);
					opaqueDirectCommandList->IASetIndexBuffer(&meshes[wallModel.meshIndex]->indexBufferView);
					opaqueDirectCommandList->DrawIndexedInstanced(meshes[wallModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
					++renderCount;
				}
			}

			waterRenderTargetTextureState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			if (iceModel.isInView(assets->mainFrustum))
			{
				auto tempRenderTargetTexturesCpuDescriptorHandle = renderTargetTexturesCpuDescriptorHandle;
				tempRenderTargetTexturesCpuDescriptorHandle.ptr += 2u * assets->graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize;
				opaqueDirectCommandList->OMSetRenderTargets(1u, &tempRenderTargetTexturesCpuDescriptorHandle, TRUE, &depthStencilHandle);
				opaqueDirectCommandList->ClearRenderTargetView(tempRenderTargetTexturesCpuDescriptorHandle, clearColor, 0u, nullptr);
				opaqueDirectCommandList->ClearDepthStencilView(depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u, 0u, nullptr);

				if (groundModel.isInView(assets->mainFrustum))
				{
					opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.pointLightPSO1);

					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, groundModel.vsPerObjectCBVGpuAddress);
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, groundModel.psPerObjectCBVGpuAddress);
					opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[groundModel.meshIndex]->vertexBufferView);
					opaqueDirectCommandList->IASetIndexBuffer(&meshes[groundModel.meshIndex]->indexBufferView);
					opaqueDirectCommandList->DrawIndexedInstanced(meshes[groundModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
					++renderCount;
				}

				opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.lightPSO1);
				if (wallModel.isInView(assets->mainFrustum))
				{
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsPerObjectCBVGpuAddress);
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, wallModel.psPerObjectCBVGpuAddress);
					opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[wallModel.meshIndex]->vertexBufferView);
					opaqueDirectCommandList->IASetIndexBuffer(&meshes[wallModel.meshIndex]->indexBufferView);
					opaqueDirectCommandList->DrawIndexedInstanced(meshes[wallModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
					++renderCount;
				}

				if (bathModel1.isInView(assets->mainFrustum))
				{
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, bathModel1.vsPerObjectCBVGpuAddress);
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, bathModel1.psPerObjectCBVGpuAddress);
					opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[bathModel1.meshIndex]->vertexBufferView);
					opaqueDirectCommandList->IASetIndexBuffer(&meshes[bathModel1.meshIndex]->indexBufferView);
					opaqueDirectCommandList->DrawIndexedInstanced(meshes[bathModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
					++renderCount;
				}

				if (fireModel1.isInView(assets->mainFrustum) || fireModel2.isInView(assets->mainFrustum))
				{
					opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.firePSO1);

					opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[fireModel1.meshIndex]->vertexBufferView);
					opaqueDirectCommandList->IASetIndexBuffer(&meshes[fireModel1.meshIndex]->indexBufferView);

					if (fireModel1.isInView(assets->mainFrustum))
					{
						opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, fireModel1.vsConstantBufferGPU(frameIndex));
						opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, fireModel1.psConstantBufferGPU());
						opaqueDirectCommandList->DrawIndexedInstanced(meshes[fireModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
						++renderCount;
					}

					if (fireModel2.isInView(assets->mainFrustum))
					{
						opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, fireModel2.vsConstantBufferGPU(frameIndex));
						opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, fireModel2.psConstantBufferGPU());
						opaqueDirectCommandList->DrawIndexedInstanced(meshes[fireModel1.meshIndex]->indexCount, 1u, 0u, 0, 0u);
						++renderCount;
					}
				}

				if (waterModel.isInView(assets->mainFrustum))
				{
					D3D12_RESOURCE_BARRIER barriers[2];
					barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barriers[0].Transition.pResource = renderTargetTextures[0];
					barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
					barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

					barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barriers[1].Transition.pResource = renderTargetTextures[1];
					barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
					barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

					opaqueDirectCommandList->ResourceBarrier(2u, barriers);
					waterRenderTargetTextureState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

					opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.waterPSO1);

					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsPerObjectCBVGpuAddresses[frameIndex]);
					opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, waterModel.psPerObjectCBVGpuAddress);
					opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &meshes[waterModel.meshIndex]->vertexBufferView);
					opaqueDirectCommandList->IASetIndexBuffer(&meshes[waterModel.meshIndex]->indexBufferView);
					opaqueDirectCommandList->DrawIndexedInstanced(meshes[waterModel.meshIndex]->indexCount, 1u, 0u, 0, 0u);
					++renderCount;
				}
			}
			if (waterRenderTargetTextureState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				afterTransBarriers[afterBarrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				afterTransBarriers[afterBarrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
				afterTransBarriers[afterBarrierCount].Transition.pResource = renderTargetTextures[0];
				afterTransBarriers[afterBarrierCount].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				afterTransBarriers[afterBarrierCount].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				afterTransBarriers[afterBarrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				++afterBarrierCount;

				afterTransBarriers[afterBarrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				afterTransBarriers[afterBarrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
				afterTransBarriers[afterBarrierCount].Transition.pResource = renderTargetTextures[1];
				afterTransBarriers[afterBarrierCount].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				afterTransBarriers[afterBarrierCount].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				afterTransBarriers[afterBarrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				++afterBarrierCount;
			}

			opaqueDirectCommandList->ResourceBarrier(afterBarrierCount, afterTransBarriers);
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


		MDResources(BaseExecutor* const executor, void* zone) :
			light(DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, -0.894427191f, 0.447213595f)),
			perObjectConstantBuffers(executor->sharedResources->graphicsEngine.graphicsDevice, []()
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

			textureManager.unloadTexture(TextureNames::marble01, executor);

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

			if (bathModel.isInView(assets->mainFrustum))
			{
				commandList->SetPipelineState(assets->pipelineStateObjects.lightPSO1);

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