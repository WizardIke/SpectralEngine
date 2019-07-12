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
		VertexShaderRequest(const wchar_t* filename, File file, std::size_t start, std::size_t end,
			void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data),
			void(*deleteRequest)(ReadRequest& request, void* tr)) :
			AsynchronousFileManager::ReadRequest(filename, file, start, end, fileLoadedCallback, deleteRequest) {}
	};
	class PixelShaderRequest : public AsynchronousFileManager::ReadRequest 
	{
	public:
		PixelShaderRequest(const wchar_t* filename, File file, std::size_t start, std::size_t end,
			void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data),
			void(*deleteRequest)(ReadRequest& request, void* tr)) :
			AsynchronousFileManager::ReadRequest(filename, file, start, end, fileLoadedCallback, deleteRequest) {}
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
				vertexShaderRequest.file.close();
				auto& pixelShaderRequest = *static_cast<PixelShaderRequest*>(this);
				pixelShaderRequest.file.close();
				asynchronousFileManager.discard(vertexShaderRequest);
				asynchronousFileManager.discard(pixelShaderRequest);
			}
		}

		PsoWithVertexAndPixelShaderRequest(const wchar_t* vertexShaderFileName, File vertexShaderFile, const wchar_t* pixelShaderFileName, File pixelShaderFile, GraphicsPipelineStateDesc& graphicsPipelineStateDesc,
			ID3D12Device& grphicsDevice, void(*psoLoadedCallback1)(PsoWithVertexAndPixelShaderRequest& request, D3D12PipelineState pso, void* tr)) :
			VertexShaderRequest(vertexShaderFileName, vertexShaderFile, 0u, vertexShaderFile.size(), [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* data)
				{
					auto& psoRequest = static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<VertexShaderRequest&>(request));
					psoRequest.graphicsPipelineStateDesc.VS.pShaderBytecode = data;
					psoRequest.graphicsPipelineStateDesc.VS.BytecodeLength = request.end;
					psoRequest.componentLoaded(asynchronousFileManager);
				}, [](AsynchronousFileManager::ReadRequest& request, void* tr)
				{
					static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<VertexShaderRequest&>(request)).deleteComponent(tr);
				}),
			PixelShaderRequest(pixelShaderFileName, pixelShaderFile, 0u, pixelShaderFile.size(), [](AsynchronousFileManager::ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void*, const unsigned char* data)
				{
					auto& psoRequest = static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<PixelShaderRequest&>(request));
					psoRequest.graphicsPipelineStateDesc.PS.pShaderBytecode = data;
					psoRequest.graphicsPipelineStateDesc.PS.BytecodeLength = request.end;
					psoRequest.componentLoaded(asynchronousFileManager);
				}, [](AsynchronousFileManager::ReadRequest& request, void* tr)
				{
					static_cast<PsoWithVertexAndPixelShaderRequest&>(static_cast<PixelShaderRequest&>(request)).deleteComponent(tr);
				}),
			graphicsPipelineStateDesc(graphicsPipelineStateDesc),
			device(grphicsDevice),
			psoLoadedCallback(psoLoadedCallback1) {}
	public:
		PsoWithVertexAndPixelShaderRequest(GraphicsPipelineStateDesc& graphicsPipelineStateDesc, AsynchronousFileManager& asynchronousFileManager, ID3D12Device& grphicsDevice,
			void(*psoLoadedCallback1)(PsoWithVertexAndPixelShaderRequest& request, D3D12PipelineState pso, void* tr)) :
			PsoWithVertexAndPixelShaderRequest(graphicsPipelineStateDesc.vertexShaderFileName,
				asynchronousFileManager.openFileForReading(graphicsPipelineStateDesc.vertexShaderFileName),
				graphicsPipelineStateDesc.pixelShaderFileName,
				asynchronousFileManager.openFileForReading(graphicsPipelineStateDesc.pixelShaderFileName),
				graphicsPipelineStateDesc,
				grphicsDevice,
				psoLoadedCallback1)
		{}

		void load(AsynchronousFileManager& asynchronousFileManager)
		{
			asynchronousFileManager.readFile(*static_cast<VertexShaderRequest*>(this));
			asynchronousFileManager.readFile(*static_cast<PixelShaderRequest*>(this));
		}
	};

	static void loadPsoWithVertexAndPixelShaders(PsoWithVertexAndPixelShaderRequest& request, AsynchronousFileManager& asynchronousFileManager)
	{
		request.load(asynchronousFileManager);
	}
};