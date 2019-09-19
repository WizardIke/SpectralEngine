#pragma once
#include "AsynchronousFileManager.h"
#include "GraphicsPipelineStateDesc.h"
#include "D3D12PipelineState.h"
#include <atomic>

class PsoLoader
{
	class VertexShaderRequest : public AsynchronousFileManager::ReadRequest 
	{
	public:
		using AsynchronousFileManager::ReadRequest::ReadRequest;
	};
	class PixelShaderRequest : public AsynchronousFileManager::ReadRequest 
	{
	public:
		using AsynchronousFileManager::ReadRequest::ReadRequest;
	};
public:
	class PsoWithVertexAndPixelShaderRequest : public VertexShaderRequest, public PixelShaderRequest
	{
		void(*psoLoadedCallback)(PsoWithVertexAndPixelShaderRequest& request, D3D12PipelineState pso, void* tr);
		ID3D12Device& device;
		GraphicsPipelineStateDesc& graphicsPipelineStateDesc;
		D3D12PipelineState pso;

		constexpr static unsigned int numberOfComponents = 2u;
		std::atomic<unsigned int> numberOfComponentsFinished = 0u;

		void deleteComponent(void* tr)
		{
			if (numberOfComponentsFinished.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
			{
				psoLoadedCallback(*this, std::move(pso), tr);
			}
		}

		void componentLoaded(AsynchronousFileManager& asynchronousFileManager)
		{
			if (numberOfComponentsFinished.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
			{
				numberOfComponentsFinished.store(0u, std::memory_order_relaxed);
				pso = D3D12PipelineState(&device, graphicsPipelineStateDesc);
				auto& vertexShaderRequest = *static_cast<VertexShaderRequest*>(this);
				auto& pixelShaderRequest = *static_cast<PixelShaderRequest*>(this);
				asynchronousFileManager.discard(vertexShaderRequest);
				asynchronousFileManager.discard(pixelShaderRequest);
			}
		}
	public:
		PsoWithVertexAndPixelShaderRequest(GraphicsPipelineStateDesc& graphicsPipelineStateDesc, ID3D12Device& grphicsDevice,
			void(*psoLoadedCallback1)(PsoWithVertexAndPixelShaderRequest& request, D3D12PipelineState pso, void* tr)) :
			VertexShaderRequest{ graphicsPipelineStateDesc.vertexShaderResource.start, graphicsPipelineStateDesc.vertexShaderResource.end, 
					[](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* data)
				{
					auto& psoRequest = static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<VertexShaderRequest&>(request));
					psoRequest.graphicsPipelineStateDesc.VS.pShaderBytecode = data;
					psoRequest.graphicsPipelineStateDesc.VS.BytecodeLength = static_cast<SIZE_T>(request.end - request.start);
					psoRequest.componentLoaded(asynchronousFileManager);
				}, [](AsynchronousFileManager::ReadRequest& request, void* tr)
				{
					static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<VertexShaderRequest&>(request)).deleteComponent(tr);
				} },
			PixelShaderRequest{ graphicsPipelineStateDesc.pixelShaderResource.start, graphicsPipelineStateDesc.pixelShaderResource.end, 
					[](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* data)
				{
					auto& psoRequest = static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<PixelShaderRequest&>(request));
					psoRequest.graphicsPipelineStateDesc.PS.pShaderBytecode = data;
					psoRequest.graphicsPipelineStateDesc.PS.BytecodeLength = static_cast<SIZE_T>(request.end - request.start);
					psoRequest.componentLoaded(asynchronousFileManager);
				}, [](AsynchronousFileManager::ReadRequest& request, void* tr)
				{
					static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<PixelShaderRequest&>(request)).deleteComponent(tr);
				} },
			graphicsPipelineStateDesc(graphicsPipelineStateDesc),
			device(grphicsDevice),
			psoLoadedCallback(psoLoadedCallback1)
		{}

		void load(AsynchronousFileManager& asynchronousFileManager)
		{
			asynchronousFileManager.read(*static_cast<VertexShaderRequest*>(this));
			asynchronousFileManager.read(*static_cast<PixelShaderRequest*>(this));
		}
	};

	static void loadPsoWithVertexAndPixelShaders(PsoWithVertexAndPixelShaderRequest& request, AsynchronousFileManager& asynchronousFileManager)
	{
		request.load(asynchronousFileManager);
	}
};