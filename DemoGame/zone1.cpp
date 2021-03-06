#include "zone1.h"
#include <GraphicsEngine.h>
#include <HresultException.h>
#include <TextureManager.h>
#include <MeshManager.h>
#include <Vector2.h>
#include <D3D12Resource.h>
#include <d3d12.h>
#include <Mesh.h>
#include "ThreadResources.h"
#include "GlobalResources.h"
#include "PipelineStateObjects.h"
#include "Resources.h"
#include <TemplateFloat.h>
#include <D3D12DescriptorHeap.h>
#include <Vector4.h>
#include <ReflectionCamera.h>
#include <Range.h>
#include "StreamingRequests.h"
#include <new>
#include <atomic>

#include <Light.h>
#include <PointLight.h>

#include "Models/BathModel2.h"
#include "Models/GroundModel2.h"
#include "Models/WallModel2.h"
#include "Models/WaterModel2.h"
#include "Models/IceModel2.h"
#include "Models/FireModel.h"

namespace
{
	class HDResources
	{
		constexpr static unsigned char numMeshes = 8u;
		constexpr static unsigned char numTextures = 9u;
		constexpr static unsigned int numPointLights = 4u;
		constexpr static unsigned char numComponents = 4u; //loading camera, setting constant buffers, loading textures, loading meshes


		static void componentUploaded(Zone<ThreadResources>& zone)
		{
			auto& globalResource = *static_cast<GlobalResources*>(zone.context);
			zone.componentUploaded(globalResource.taskShedular, numComponents);
		}
	public:
		Mesh* squareWithPos;
		Mesh* cubeWithPos;
		D3D12Resource perObjectConstantBuffers;
		unsigned char* perObjectConstantBuffersCpuAddress;
		//D3D12_RESOURCE_STATES waterRenderTargetTextureState;
		unsigned long zoneEntity; //used to attach components e.g. cameras

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

		HDResources(ThreadResources& threadResources, GlobalResources& globalResources, Zone<ThreadResources>& zone) :
			light({ 0.1f, 0.1f, 0.1f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -0.894427191f, 0.447213595f }),

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
			pointLights[0u] = PointLight{ Vector4{ 20.0f, 2.0f, 2.0f, 1.0f }, Vector4{ 52.0f, 5.0f, 76.0f, 1.f } };
			pointLights[1u] = PointLight{ Vector4{ 2.0f, 20.0f, 2.0f, 1.0f }, Vector4{ 76.0f, 5.0f, 76.0f, 1.f } };
			pointLights[2u] = PointLight{ Vector4{ 2.0f, 2.0f, 20.0f, 1.0f }, Vector4{ 52.0f, 5.0f, 52.0f, 1.f } };
			pointLights[3u] = PointLight{ Vector4{ 20.0f, 20.0f, 20.0f, 1.0f }, Vector4{ 76.0f, 5.0f, 52.0f, 1.f } };

			D3D12_RANGE readRange{ 0u, 0u };
			HRESULT hr = perObjectConstantBuffers->Map(0u, &readRange, reinterpret_cast<void**>(&perObjectConstantBuffersCpuAddress));
			if (FAILED(hr)) throw HresultException(hr);
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();
			unsigned char* cpuConstantBuffer = perObjectConstantBuffersCpuAddress;

			zoneEntity = globalResources.entityGenerator.generate();
			unsigned int reflectionCameraBackBufferIndex;
			{
				auto depthStencilHandle = globalResources.graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				Transform transform = globalResources.mainCamera().transform().reflection(waterModel.reflectionHeight());

				AddReflectionCamera* addCameraRequest = new AddReflectionCamera(zoneEntity, ReflectionCamera(depthStencilHandle, globalResources.window.width(), globalResources.window.height(), PerObjectConstantBuffersGpuAddress,
					cpuConstantBuffer, 0.25f * 3.141f, transform, globalResources.graphicsEngine), [](auto& request, void*)
					{
						AddReflectionCamera& addRequest = static_cast<AddReflectionCamera&>(request);
						auto& zone = addRequest.zone;
						delete &addRequest;
						componentUploaded(zone);
					}, zone);
				reflectionCameraBackBufferIndex = addCameraRequest->camera.getShaderResourceViewIndex();
				globalResources.renderPass.renderToTextureSubPass().addCameraFromBackground(*addCameraRequest, globalResources.renderPass, globalResources.taskShedular);
			}

			bathModel1.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			groundModel.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			wallModel.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			waterModel.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer, reflectionCameraBackBufferIndex,
				globalResources.refractionRenderer.shaderResourceViewDescriptorIndex());

			iceModel.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer, globalResources.refractionRenderer.shaderResourceViewDescriptorIndex());

			fireModel1.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			fireModel2.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			auto textureRequests = new TextureRequests<numTextures>([](TextureRequests<numTextures>& request, ThreadResources&)
			{
				auto& zone = request.zone();
				delete &request;
				componentUploaded(zone);
			});
			textureRequests->load(
				{
					{
						Resources::Textures::ground01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->groundModel.setDiffuseTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::wall01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->wallModel.setDiffuseTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::marble01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->bathModel1.setDiffuseTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::water01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->waterModel.setNormalTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::ice01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->iceModel.setDiffuseTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::icebump01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->iceModel.setNormalTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::firenoise01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->fireModel1.setNormalTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
							resources->fireModel2.setNormalTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::fire01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->fireModel1.setDiffuseTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
							resources->fireModel2.setDiffuseTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
					{
						Resources::Textures::firealpha01,
						[](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
						{
							auto resources = static_cast<HDResources*>(static_cast<TextureRequest&>(request).zone.newData);
							resources->fireModel1.setAlphaTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
							resources->fireModel2.setAlphaTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());
						}
					},
				}, zone, threadResources, globalResources.textureManager);

			auto meshRequests = new MeshRequests<numMeshes>([](MeshRequests<numMeshes>& request, ThreadResources&)
			{
				auto& zone = request.zone();
				delete &request;
				componentUploaded(zone);
			});
			meshRequests->load(
				{
					{
						Resources::Meshes::bathWithNormals,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->bathModel1.mesh = &mesh;
						}
					},
					{
						Resources::Meshes::plane,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->groundModel.mesh = &mesh;
						}
					},
					{
						Resources::Meshes::wall,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->wallModel.mesh = &mesh;
						}
					},
					{
						Resources::Meshes::water,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->waterModel.mesh = &mesh;
						}
					},
					{
						Resources::Meshes::cube,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->iceModel.mesh = &mesh;
						}
					},
					{
						Resources::Meshes::square,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->fireModel1.mesh = &mesh;
							resources->fireModel2.mesh = &mesh;
						}
					},
					{
						Resources::Meshes::aabb,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->cubeWithPos = &mesh;
						}
					},
					{
						Resources::Meshes::squareWithPositions,
						[](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
						{
							auto resources = static_cast<HDResources*>(static_cast<MeshRequest&>(request).zone.newData);
							resources->squareWithPos = &mesh;
						}
					},
				}, zone, threadResources, globalResources.meshManager);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) & 
				~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);
			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(cpuConstantBuffer);
			cpuConstantBuffer += frameBufferCount * pointLightConstantBufferAlignedSize;

			pointLightConstantBufferGpuAddress = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += frameBufferCount * pointLightConstantBufferAlignedSize;
			//sound3D1.Play(DSBPLAY_LOOPING);
		}

		~HDResources() {}

		static void create(void* context, ThreadResources& threadResources)
		{
			Zone<ThreadResources>& zone = *static_cast<Zone<ThreadResources>*>(context);
			auto& globalResources = *static_cast<GlobalResources*>(zone.context);
			zone.newData = operator new(sizeof(HDResources));
			new(zone.newData) HDResources(threadResources, globalResources, zone);
			componentUploaded(zone);
		}

		void update1(ThreadResources&, GlobalResources& globalResources)
		{
			const auto frameTime = globalResources.timer.frameTime();
			waterModel.update(frameTime);
			fireModel1.update(frameTime);
			fireModel2.update(frameTime);
		}

		void updateConstantBuffers(GlobalResources& globalResources)
		{
			const auto frameIndex = globalResources.graphicsEngine.frameIndex;
			const auto& cameraPos = globalResources.mainCamera().position();
			waterModel.beforeRender(globalResources.graphicsEngine);
			fireModel1.beforeRender(frameIndex, cameraPos);
			fireModel2.beforeRender(frameIndex, cameraPos);
			auto& reflectionCamera = *globalResources.renderPass.renderToTextureSubPass().findCamera(zoneEntity);
			reflectionCamera.beforeRender(frameIndex, globalResources.mainCamera().transform().reflection(waterModel.reflectionHeight()).toMatrix());

			const auto frameTime = globalResources.timer.frameTime();
			auto rotationMatrix = DirectX::XMMatrixTranslation(-64.0f, -5.0f, -64.0f) * DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), frameTime) * DirectX::XMMatrixTranslation(64.0f, 5.0f, 64.0f);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
			auto pointLightConstantBuffer = reinterpret_cast<LightConstantBuffer*>(reinterpret_cast<unsigned char*>(pointLightConstantBufferCpuAddress) + frameIndex * pointLightConstantBufferAlignedSize);

			pointLightConstantBuffer->ambientLight = light.ambientLight;
			pointLightConstantBuffer->directionalLight = light.directionalLight;
			pointLightConstantBuffer->lightDirection = light.direction;

			pointLightConstantBuffer->pointLightCount = numPointLights;
			for(auto i = 0u; i < numPointLights; ++i)
			{
				auto temp = DirectX::XMVector4Transform(DirectX::XMVectorSet(pointLights[i].position.x(), pointLights[i].position.y(),
					pointLights[i].position.z(), pointLights[i].position.w()), rotationMatrix);
				DirectX::XMFLOAT4 temp2;
				DirectX::XMStoreFloat4(&temp2, temp);
				pointLights[i].position.x() = temp2.x; pointLights[i].position.y() = temp2.y; pointLights[i].position.z() = temp2.z; pointLights[i].position.w() = temp2.w;
				pointLightConstantBuffer->pointLights[i] = pointLights[i];
			}
		}

		void renderToTexture(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			const auto frameIndex = globalResources.graphicsEngine.frameIndex;

			auto depthStencilHandle = globalResources.graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			if (waterModel.isInView(globalResources.mainCamera().frustum()))
			{
				auto& camera = *globalResources.renderPass.renderToTextureSubPass().findCamera(zoneEntity);
				const auto& frustum = camera.frustum();
				const auto commandList = threadResources.renderPass.renderToTextureSubPass().firstCommandList();
				auto warpTextureCpuDescriptorHandle = globalResources.refractionRenderer.renderTargetDescriptorHandle();
				auto backBufferRenderTargetCpuHandle = camera.getRenderTargetView();

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
				commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress + frameIndex * pointLightConstantBufferAlignedSize);

				D3D12_RESOURCE_BARRIER copyStartBarriers[2u];
				copyStartBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				copyStartBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				copyStartBarriers[0u].Transition.pResource = &globalResources.refractionRenderer.resource();
				copyStartBarriers[0u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				copyStartBarriers[0u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				copyStartBarriers[0u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				copyStartBarriers[1u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				copyStartBarriers[1u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				copyStartBarriers[1u].Transition.pResource = &camera.getImage();
				copyStartBarriers[1u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
				copyStartBarriers[1u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				copyStartBarriers[1u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				D3D12_RESOURCE_BARRIER copyEndBarriers[2u];
				copyEndBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				copyEndBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				copyEndBarriers[0u].Transition.pResource = &globalResources.refractionRenderer.resource();
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
					commandList->SetPipelineState(globalResources.pipelineStateObjects.directionalLight);
					commandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsBufferGpu());
					commandList->SetGraphicsRootConstantBufferView(3u, wallModel.psBufferGpu());
					commandList->IASetVertexBuffers(0u, 1u, &wallModel.mesh->vertexBufferView);
					commandList->IASetIndexBuffer(&wallModel.mesh->indexBufferView);
					commandList->DrawIndexedInstanced(wallModel.mesh->indexCount(), 1u, 0u, 0, 0u);
				}
				if (iceModel.isInView(frustum))
				{
					commandList->ResourceBarrier(2u, copyStartBarriers);
					commandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
					commandList->SetPipelineState(globalResources.pipelineStateObjects.copy);
					commandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsAabbGpu());
					commandList->IASetVertexBuffers(0u, 1u, &cubeWithPos->vertexBufferView);
					commandList->IASetIndexBuffer(&cubeWithPos->indexBufferView);
					commandList->DrawIndexedInstanced(cubeWithPos->indexCount(), 1u, 0u, 0, 0u);

					commandList->ResourceBarrier(2u, copyEndBarriers);
					commandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
					commandList->SetPipelineState(globalResources.pipelineStateObjects.glass);
					commandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsConstantBufferGPU(frameIndex));
					commandList->SetGraphicsRootConstantBufferView(3u, iceModel.psConstantBufferGPU());
					commandList->IASetVertexBuffers(0u, 1u, &iceModel.mesh->vertexBufferView);
					commandList->IASetIndexBuffer(&iceModel.mesh->indexBufferView);
					commandList->DrawIndexedInstanced(iceModel.mesh->indexCount(), 1u, 0u, 0, 0u);
				}
			}
		}

		void update2(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			updateConstantBuffers(globalResources);
			renderToTexture(threadResources, globalResources);

			const auto frameIndex = globalResources.graphicsEngine.frameIndex;
			const auto opaqueDirectCommandList = threadResources.renderPass.colorSubPass().opaqueCommandList();
			const auto transparantCommandList = threadResources.renderPass.colorSubPass().transparentCommandList();
			auto depthStencilHandle = globalResources.graphicsEngine.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			auto backBufferRenderTargetCpuHandle = globalResources.mainCamera().getRenderTargetView(frameIndex);
			auto warpTextureCpuDescriptorHandle = globalResources.refractionRenderer.renderTargetDescriptorHandle();

			D3D12_RESOURCE_BARRIER copyStartBarriers[2u];
			copyStartBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyStartBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			copyStartBarriers[0u].Transition.pResource = &globalResources.refractionRenderer.resource();
			copyStartBarriers[0u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			copyStartBarriers[0u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			copyStartBarriers[0u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			copyStartBarriers[1u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyStartBarriers[1u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			copyStartBarriers[1u].Transition.pResource = globalResources.window.getBuffer(frameIndex);
			copyStartBarriers[1u].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			copyStartBarriers[1u].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			copyStartBarriers[1u].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			D3D12_RESOURCE_BARRIER copyEndBarriers[2u];
			copyEndBarriers[0u].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyEndBarriers[0u].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			copyEndBarriers[0u].Transition.pResource = &globalResources.refractionRenderer.resource();
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
			const auto& frustum = globalResources.mainCamera().frustum();

			if (groundModel.isInView(frustum))
			{
				opaqueDirectCommandList->SetPipelineState(globalResources.pipelineStateObjects.pointLight);

				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, groundModel.vsBufferGpu());
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, groundModel.psBufferGpu());
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &groundModel.mesh->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&groundModel.mesh->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(groundModel.mesh->indexCount(), 1u, 0u, 0, 0u);
			}

			opaqueDirectCommandList->SetPipelineState(globalResources.pipelineStateObjects.directionalLight);
			if (wallModel.isInView(frustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, wallModel.vsBufferGpu());
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, wallModel.psBufferGpu());
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &wallModel.mesh->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&wallModel.mesh->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(wallModel.mesh->indexCount(), 1u, 0u, 0, 0u);
			}

			if (bathModel1.isInView(frustum))
			{
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(2u, bathModel1.vsBufferGpu());
				opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, bathModel1.psBufferGpu());
				opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &bathModel1.mesh->vertexBufferView);
				opaqueDirectCommandList->IASetIndexBuffer(&bathModel1.mesh->indexBufferView);
				opaqueDirectCommandList->DrawIndexedInstanced(bathModel1.mesh->indexCount(), 1u, 0u, 0, 0u);
			}

			transparantCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			if (waterModel.isInView(frustum))
			{
				transparantCommandList->ResourceBarrier(2u, copyStartBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(globalResources.pipelineStateObjects.copy);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsAabbGpu());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &squareWithPos->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&squareWithPos->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(squareWithPos->indexCount(), 1u, 0u, 0, 0u);

				transparantCommandList->ResourceBarrier(2u, copyEndBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(globalResources.pipelineStateObjects.waterWithReflectionTexture);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, waterModel.vsConstantBufferGPU(frameIndex));
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, waterModel.psConstantBufferGPU());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &waterModel.mesh->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&waterModel.mesh->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(waterModel.mesh->indexCount(), 1u, 0u, 0, 0u);
			}


			if (iceModel.isInView(frustum))
			{
				transparantCommandList->ResourceBarrier(2u, copyStartBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &warpTextureCpuDescriptorHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(globalResources.pipelineStateObjects.copy);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsAabbGpu());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &cubeWithPos->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&cubeWithPos->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(cubeWithPos->indexCount(), 1u, 0u, 0, 0u);

				transparantCommandList->ResourceBarrier(2u, copyEndBarriers);
				transparantCommandList->OMSetRenderTargets(1u, &backBufferRenderTargetCpuHandle, TRUE, &depthStencilHandle);
				transparantCommandList->SetPipelineState(globalResources.pipelineStateObjects.glass);
				transparantCommandList->SetGraphicsRootConstantBufferView(2u, iceModel.vsConstantBufferGPU(frameIndex));
				transparantCommandList->SetGraphicsRootConstantBufferView(3u, iceModel.psConstantBufferGPU());
				transparantCommandList->IASetVertexBuffers(0u, 1u, &iceModel.mesh->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&iceModel.mesh->indexBufferView);
				transparantCommandList->DrawIndexedInstanced(iceModel.mesh->indexCount(), 1u, 0u, 0, 0u);
			}
			
			if (fireModel1.isInView(frustum) || fireModel2.isInView(frustum))
			{
				transparantCommandList->SetPipelineState(globalResources.pipelineStateObjects.fire);

				transparantCommandList->IASetVertexBuffers(0u, 1u, &fireModel1.mesh->vertexBufferView);
				transparantCommandList->IASetIndexBuffer(&fireModel1.mesh->indexBufferView);

				if (fireModel1.isInView(frustum))
				{
					transparantCommandList->SetGraphicsRootConstantBufferView(2u, fireModel1.vsConstantBufferGPU(frameIndex));
					transparantCommandList->SetGraphicsRootConstantBufferView(3u, fireModel1.psConstantBufferGPU());
					transparantCommandList->DrawIndexedInstanced(fireModel1.mesh->indexCount(), 1u, 0u, 0, 0u);
				}

				if (fireModel2.isInView(frustum))
				{
					transparantCommandList->SetGraphicsRootConstantBufferView(2u, fireModel2.vsConstantBufferGPU(frameIndex));
					transparantCommandList->SetGraphicsRootConstantBufferView(3u, fireModel2.psConstantBufferGPU());
					transparantCommandList->DrawIndexedInstanced(fireModel2.mesh->indexCount(), 1u, 0u, 0, 0u);
				}
			}
		}

		class HdUnloader : private TextureUnloader<numTextures>, private MeshUnloader<numMeshes>
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
					auto& globalResources = *static_cast<GlobalResources*>(zoneRef.context);
					zoneRef.finishedDeletingOldState(globalResources.taskShedular);
					resource->~HDResources();
					operator delete(resource);
				}
			}

			HdUnloader(Zone<ThreadResources>& zone1) :
				TextureUnloader<numTextures>([](TextureUnloader<numTextures>& unloader, void* tr)
					{
						static_cast<HdUnloader&>(unloader).componentUnloaded(tr);
					}),
				MeshUnloader<numMeshes>([](MeshUnloader<numMeshes>& unloader, void* tr)
					{
						static_cast<HdUnloader&>(unloader).componentUnloaded(tr);
					}),
						zone(zone1)
					{}
		public:
			static HdUnloader* create(Zone<ThreadResources>& zone1)
			{
				return new HdUnloader(zone1);
			}

			void unloadTextures(const ResourceLocation (&filenames)[numTextures], ThreadResources& threadResources, TextureManager& textureManager)
			{
				static_cast<TextureUnloader<numTextures>&>(*this).unload(filenames, threadResources, textureManager);
			}

			void unloadMeshes(const ResourceLocation (&filenames)[numMeshes], ThreadResources& threadResources, MeshManager& meshManager)
			{
				static_cast<MeshUnloader<numMeshes>&>(*this).unload(filenames, threadResources, meshManager);
			}
		};

		void destruct(Zone<ThreadResources>& zone, ThreadResources& threadResources)
		{
			auto& globalResources = *static_cast<GlobalResources*>(zone.context);

			perObjectConstantBuffers->Unmap(0u, nullptr);

			auto hdUnloader = HdUnloader::create(zone);
			hdUnloader->unloadTextures(
				{
					Resources::Textures::ground01,
					Resources::Textures::wall01,
					Resources::Textures::marble01,
					Resources::Textures::water01,
					Resources::Textures::ice01,
					Resources::Textures::icebump01,
					Resources::Textures::firenoise01,
					Resources::Textures::fire01,
					Resources::Textures::firealpha01,
				}, threadResources, globalResources.textureManager);

			hdUnloader->unloadMeshes(
				{
					Resources::Meshes::bathWithNormals,
					Resources::Meshes::plane,
					Resources::Meshes::wall,
					Resources::Meshes::water,
					Resources::Meshes::cube,
					Resources::Meshes::square,
					Resources::Meshes::aabb,
					Resources::Meshes::squareWithPositions,
				}, threadResources, globalResources.meshManager);
		}
	};

	class MDResources
	{
		constexpr static unsigned int numComponents = 3u;
		constexpr static unsigned int numMeshes = 1u;
		constexpr static unsigned int numTextures = 1u;


		static void callback(Zone<ThreadResources>& zone)
		{
			auto& globalResources = *static_cast<GlobalResources*>(zone.context);
			zone.componentUploaded(globalResources.taskShedular, numComponents);
		}
	public:
		D3D12Resource perObjectConstantBuffers;
		unsigned char* perObjectConstantBuffersCpuAddress;

		BathModel2 bathModel;

		Light light;
		LightConstantBuffer* pointLightConstantBufferCpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS pointLightConstantBufferGpuAddress;


		MDResources(ThreadResources& threadResources, GlobalResources& globalResources, Zone<ThreadResources>& zone) :
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
			if (FAILED(hr)) throw HresultException(hr);
			auto PerObjectConstantBuffersGpuAddress = perObjectConstantBuffers->GetGPUVirtualAddress();
			auto cpuConstantBuffer = perObjectConstantBuffersCpuAddress;

			bathModel.setConstantBuffers(PerObjectConstantBuffersGpuAddress, cpuConstantBuffer);

			TextureManager& textureManager = globalResources.textureManager;
			TextureRequest* marble01Request = new TextureRequest([](TextureManager::TextureStreamingRequest& request, void*, unsigned int textureDescriptor)
			{
				auto& zone = static_cast<TextureRequest&>(request).zone;
				auto resources = ((MDResources*)zone.newData);
				resources->bathModel.setDiffuseTexture(textureDescriptor, resources->perObjectConstantBuffersCpuAddress, resources->perObjectConstantBuffers->GetGPUVirtualAddress());

				delete static_cast<TextureRequest*>(&request);
				callback(zone);
			}, Resources::Textures::marble01, zone);
			textureManager.load(*marble01Request, threadResources);

			MeshManager& meshManager = globalResources.meshManager;
			MeshRequest* bathRequest = new MeshRequest([](MeshManager::MeshStreamingRequest& request, void*, Mesh& mesh)
			{
				auto& zone = static_cast<MeshRequest&>(request).zone;
				auto resources = ((MDResources*)zone.newData);
				resources->bathModel.mesh = &mesh;

				delete static_cast<MeshRequest*>(&request);
				callback(zone);
			}, Resources::Meshes::bathWithNormals, zone);
			meshManager.load(bathRequest, threadResources);

			constexpr uint64_t pointLightConstantBufferAlignedSize = (sizeof(LightConstantBuffer) + (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u) &
				~((uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (uint64_t)1u);
			pointLightConstantBufferCpuAddress = reinterpret_cast<LightConstantBuffer*>(cpuConstantBuffer);
			cpuConstantBuffer += pointLightConstantBufferAlignedSize;

			pointLightConstantBufferGpuAddress = PerObjectConstantBuffersGpuAddress;
			PerObjectConstantBuffersGpuAddress += pointLightConstantBufferAlignedSize;
		}

		class MdUnloader : private TextureManager::UnloadRequest, private MeshManager::UnloadRequest
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
					auto resource = static_cast<MDResources*>(zoneRef.oldData);
					auto& globalResources = *static_cast<GlobalResources*>(zoneRef.context);
					zoneRef.finishedDeletingOldState(globalResources.taskShedular);
					resource->~MDResources();
					operator delete(resource);
				}
			}

			MdUnloader(Zone<ThreadResources>& zone1) :
				TextureManager::UnloadRequest(Resources::Textures::marble01, [](AsynchronousFileManager::ReadRequest& unloader, void* tr)
					{
						static_cast<MdUnloader&>(static_cast<TextureManager::UnloadRequest&>(unloader)).componentUnloaded(tr);
					}),
				MeshManager::UnloadRequest(Resources::Meshes::bathWithNormals, [](AsynchronousFileManager::ReadRequest& unloader, void* tr)
					{
						static_cast<MdUnloader&>(static_cast<MeshManager::UnloadRequest&>(unloader)).componentUnloaded(tr);
					}),
						zone(zone1)
					{}
		public:
			static MdUnloader* create(Zone<ThreadResources>& zone1)
			{
				return new MdUnloader(zone1);
			}

			void unload(TextureManager& textureManager, MeshManager& meshManager, ThreadResources& threadResources)
			{
				textureManager.unload(*this, threadResources);
				meshManager.unload(this, threadResources);
			}
		};

		void destruct(Zone<ThreadResources>& zone, ThreadResources& threadResources)
		{
			perObjectConstantBuffers->Unmap(0u, nullptr);

			auto& globalResources = *static_cast<GlobalResources*>(zone.context);
			auto& textureManager = globalResources.textureManager;
			auto& meshManager = globalResources.meshManager;

			auto mdUnloader = MdUnloader::create(zone);
			mdUnloader->unload(textureManager, meshManager, threadResources);
		}

		~MDResources() {}

		void update1(ThreadResources&, GlobalResources&) {}

		void update2(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			auto pointLightConstantBuffer = reinterpret_cast<LightConstantBuffer*>(pointLightConstantBufferCpuAddress);
			pointLightConstantBuffer->ambientLight = light.ambientLight;
			pointLightConstantBuffer->directionalLight = light.directionalLight;
			pointLightConstantBuffer->lightDirection = light.direction;
			pointLightConstantBuffer->pointLightCount = 0u;


			const auto commandList = threadResources.renderPass.colorSubPass().opaqueCommandList();

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			commandList->SetGraphicsRootConstantBufferView(1u, pointLightConstantBufferGpuAddress);

			if (bathModel.isInView(globalResources.mainCamera().frustum()))
			{
				commandList->SetPipelineState(globalResources.pipelineStateObjects.directionalLight);

				commandList->SetGraphicsRootConstantBufferView(2u, bathModel.vsBufferGpu());
				commandList->SetGraphicsRootConstantBufferView(3u, bathModel.psBufferGpu());
				commandList->IASetVertexBuffers(0u, 1u, &bathModel.mesh->vertexBufferView);
				commandList->IASetIndexBuffer(&bathModel.mesh->indexBufferView);
				commandList->DrawIndexedInstanced(bathModel.mesh->indexCount(), 1u, 0u, 0, 0u);
			}
		}

		static void create(void* context, ThreadResources& threadResources)
		{
			Zone<ThreadResources>& zone = *static_cast<Zone<ThreadResources>*>(context);
			auto& globalResources = *static_cast<GlobalResources*>(zone.context);
			zone.newData = operator new(sizeof(MDResources));
			new(zone.newData) MDResources(threadResources, globalResources, zone);
			callback(zone);
		}
	};

	static void update2(void* context, ThreadResources& threadResources);
	static void update1(void* context, ThreadResources& threadResources)
	{
		Zone<ThreadResources>& zone = *static_cast<Zone<ThreadResources>*>(context);
		auto& globalResources = *static_cast<GlobalResources*>(zone.context);

		zone.lastUsedData = zone.currentData;
		zone.lastUsedState = zone.currentState;

		switch (zone.currentState)
		{
		case 0u:
			static_cast<HDResources*>(zone.currentData)->update1(threadResources, globalResources);
			threadResources.taskShedular.pushPrimaryTask(1u, { &zone, update2 });
			break;
		case 1u:
			static_cast<MDResources*>(zone.currentData)->update1(threadResources, globalResources);
			threadResources.taskShedular.pushPrimaryTask(1u, { &zone, update2 });
			break;
		}
	}

	static void update2(void* context, ThreadResources& threadResources)
	{
		Zone<ThreadResources>& zone = *static_cast<Zone<ThreadResources>*>(context);
		auto& globalResources = *static_cast<GlobalResources*>(zone.context);

		switch (zone.lastUsedState)
		{
		case 0u:
			static_cast<HDResources*>(zone.lastUsedData)->update2(threadResources, globalResources);
			break;
		case 1u:
			static_cast<MDResources*>(zone.lastUsedData)->update2(threadResources, globalResources);
			break;
		}

		threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1});
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
			case 1u:
				threadResources.taskShedular.pushBackgroundTask({ &zone, &MDResources::create });
				break;
			}
		}

		static void deleteOldStateData(Zone<ThreadResources>& zone, ThreadResources& threadResources)
		{
			switch (zone.oldState)
			{
			case 0u:
			{
				auto resource = static_cast<HDResources*>(zone.oldData);
				auto zoneEntity = resource->zoneEntity;
				RemoveReflectionCameraRequest* removeReflectionCameraRequest = new RemoveReflectionCameraRequest(zoneEntity,
					[](RenderPass1::RenderToTextureSubPass::RemoveCamerasRequest& req, void* tr)
					{
						auto& request = static_cast<RemoveReflectionCameraRequest&>(req);
						ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
						threadResources.taskShedular.pushPrimaryTask(1u, { &request, [](void* requester, ThreadResources&)
						{
							auto& request = *static_cast<RemoveReflectionCameraRequest*>(requester);
							auto& zone = request.zone;
							GraphicsEngine& graphicsEngine = static_cast<GlobalResources*>(zone.context)->graphicsEngine;

							request.execute = [](LinkedTask& task, void* tr)
							{
								RemoveReflectionCameraRequest& request = static_cast<RemoveReflectionCameraRequest&>(task);
								auto& zone = request.zone;
								GraphicsEngine& graphicsEngine = static_cast<GlobalResources*>(zone.context)->graphicsEngine;
								request.camera.destruct(graphicsEngine);
								delete &request;

								ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
								threadResources.taskShedular.pushBackgroundTask({ &zone, [](void* context, ThreadResources& threadResources)
								{
									Zone<ThreadResources>& zone = *static_cast<Zone<ThreadResources>*>(context);
									auto resource = static_cast<HDResources*>(zone.oldData);
									resource->destruct(zone, threadResources);
								} });
							};
							graphicsEngine.executeWhenGpuFinishesCurrentFrame(request);
						} });
					}, zone);
				auto& globalResources = *static_cast<GlobalResources*>(zone.context);
				globalResources.renderPass.renderToTextureSubPass().removeCamera(*removeReflectionCameraRequest, globalResources.renderPass, threadResources);
				break;
			}
			case 1u:
			{
				GraphicsEngine& graphicsEngine = static_cast<GlobalResources*>(zone.context)->graphicsEngine;
				zone.executeWhenGpuFinishesCurrentFrame(graphicsEngine, [](LinkedTask& task, void* tr)
				{
					auto& zone = Zone<ThreadResources>::from(task);
					ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
					threadResources.taskShedular.pushBackgroundTask({ &zone, [](void* context, ThreadResources& threadResources)
					{
						auto& zone = *static_cast<Zone<ThreadResources>*>(context);
						auto resource = static_cast<MDResources*>(zone.oldData);
						resource->destruct(zone, threadResources);
					} });
				});
				break;
			}
			}
		}

		static Range<Portal*> getPortals(Zone<ThreadResources>&)
		{
			static Portal portals[1] = {Portal{Vector3(64.0f, 6.0f, 84.0f), Vector3(64.0f, 6.0f, 84.0f), 1u}};
			return range(portals, portals + 1u);
		}

		static void start(Zone<ThreadResources>& zone, ThreadResources& threadResources)
		{
			threadResources.taskShedular.pushPrimaryTask(0u, { &zone, update1 });
		}
	};
}

Zone<ThreadResources> Zone1(void* context)
{
	return Zone<ThreadResources>::create<Zone1Functions>(2u, context);
}