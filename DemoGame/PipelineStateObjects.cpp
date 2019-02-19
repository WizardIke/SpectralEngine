#include "PipelineStateObjects.h"
#include <d3d12.h>
#include <IOCompletionQueue.h>
#include <AsynchronousFileManager.h>
#include "GlobalResources.h"
#include "ThreadResources.h"

namespace
{
	class ShaderRequestVP
	{
		constexpr static unsigned int numberOfComponents = 2u;

		class RequestHelper : public AsynchronousFileManager::ReadRequest
		{
		public:
			ShaderRequestVP* request;
		};

		ID3D12RootSignature* rootSignature;
		ID3D12Device* device;
		PipelineStateObjects::PipelineLoader* pipelineLoader;

		RequestHelper vertexHelper;
		const unsigned char* vertexShaderData;

		RequestHelper pixelHelper;
		const unsigned char* pixelShaderData;

		std::atomic<unsigned int> numberOfComponentsLoaded = 0u;
		std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;
		void(*loadingFinished)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
			ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests);

		static void freeRequestMemory(AsynchronousFileManager::ReadRequest& request1, void*, void*)
		{
			ShaderRequestVP& request = *static_cast<RequestHelper&>(request1).request;
			if(request.numberOfComponentsReadyToDelete.fetch_add(1u) == (numberOfComponents - 1u))
			{
				delete &request;
			}
		}

		void shaderFileLoaded(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			if(numberOfComponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel) == (numberOfComponents - 1u))
			{
				loadingFinished(vertexShaderData, vertexHelper.end, pixelShaderData, pixelHelper.end,
					threadResources, globalResources, rootSignature, device, this);
				globalResources.asynchronousFileManager.discard(&vertexHelper, threadResources, globalResources);
				globalResources.asynchronousFileManager.discard(&pixelHelper, threadResources, globalResources);
			}
		}
	
		ShaderRequestVP(ThreadResources& threadResources, GlobalResources& globalResources, const wchar_t* vertexFile, const wchar_t* pixelFile, ID3D12RootSignature* rootSignature1, ID3D12Device* device1,
			PipelineStateObjects::PipelineLoader* pipelineLoader1,
			void(*loadingFinished1)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)) :
			loadingFinished(loadingFinished1),
			pipelineLoader(pipelineLoader1)
		{
			vertexHelper.request = this;
			pixelHelper.request = this;
			rootSignature = rootSignature1;
			device = device1;

			AsynchronousFileManager& fileManager = globalResources.asynchronousFileManager;
			IOCompletionQueue& ioCompletionQueue = globalResources.ioCompletionQueue;

			File vsFile = fileManager.openFileForReading<GlobalResources>(ioCompletionQueue, vertexFile);
			RequestHelper& request1 = vertexHelper;
			request1.filename = vertexFile;
			request1.file = vsFile;
			request1.start = 0u;
			request1.end = vsFile.size();
			request1.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* executor, void* sharedResources, const unsigned char* data)
			{
				request.file.close();

				ShaderRequestVP& request1 = *static_cast<RequestHelper&>(request).request;
				request1.vertexShaderData = data;
				request1.shaderFileLoaded(*static_cast<ThreadResources*>(executor), *static_cast<GlobalResources*>(sharedResources));
			};
			request1.deleteReadRequest = freeRequestMemory;
			fileManager.readFile(&request1, threadResources, globalResources);

			File psFile = fileManager.openFileForReading<GlobalResources>(ioCompletionQueue, pixelFile);
			RequestHelper& request2 = pixelHelper;
			request2.filename = pixelFile;
			request2.file = psFile;
			request2.start = 0u;
			request2.end = psFile.size();
			request2.fileLoadedCallback = [](AsynchronousFileManager::ReadRequest& request, void* executor, void* sharedResources, const unsigned char* data)
			{
				request.file.close();

				ShaderRequestVP& request1 = *static_cast<RequestHelper&>(request).request;
				request1.pixelShaderData = data;
				request1.shaderFileLoaded(*static_cast<ThreadResources*>(executor), *static_cast<GlobalResources*>(sharedResources));
			};
			request2.deleteReadRequest = freeRequestMemory;
			fileManager.readFile(&request2, threadResources, globalResources);
		}
	public:
		static ShaderRequestVP* create(ThreadResources& threadResources, GlobalResources& globalResources, const wchar_t* vertexFile, const wchar_t* pixelFile, ID3D12RootSignature* rootSignature1, ID3D12Device* device1,
			PipelineStateObjects::PipelineLoader* pipelineLoader1,
			void(*loadingFinished1)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests))
		{
			return new ShaderRequestVP(threadResources, globalResources, vertexFile, pixelFile, rootSignature1, device1, pipelineLoader1, loadingFinished1);
		}

		void componentLoaded(ThreadResources& threadResources, GlobalResources& globalResources)
		{
			pipelineLoader->componentLoaded(threadResources, globalResources);
		}
	};


	void loadTextShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/TextVS.cso", L"../DemoGame/CompiledShaders/TextPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = TRUE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.text = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.text->SetName(L"Text");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadDirectionalLightShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/LightVS.cso", L"../DemoGame/CompiledShaders/DirectionalLightPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.directionalLight = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.directionalLight->SetName(L"DirectionalLight");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadDirectionalLightVtShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/LightVS.cso", L"../DemoGame/CompiledShaders/DirectionalLightVtPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.directionalLightVt = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.directionalLightVt->SetName(L"directionalLightVt");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadDirectionalLightVtTwoSidedShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/LightVS.cso", L"../DemoGame/CompiledShaders/DirectionalLightVtPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.directionalLightVtTwoSided = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.directionalLightVtTwoSided->SetName(L"directionalLightVtTwoSided");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadPointLightShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/LightVS.cso", L"../DemoGame/CompiledShaders/PointLightPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.pointLight = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.pointLight->SetName(L"pointLight");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadWaterWithReflectionTextureShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/WaterVS.cso", L"../DemoGame/CompiledShaders/WaterWithReflectionTexturePS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = //texturedInputLayout
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.waterWithReflectionTexture = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.waterWithReflectionTexture->SetName(L"waterWithReflectionTexture");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadWaterNoReflectionTextureShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/WaterVS.cso", L"../DemoGame/CompiledShaders/WaterNoReflectionTexturePS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = //texturedInputLayout
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.waterNoReflectionTexture = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.waterNoReflectionTexture->SetName(L"waterNoReflectionTexture");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadGlassShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/GlassVS.cso", L"../DemoGame/CompiledShaders/GlassPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = //texturedInputLayout
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.glass = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.glass->SetName(L"glass");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadBasicShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/BasicVS.cso", L"../DemoGame/CompiledShaders/BasicPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = //texturedInputLayout
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R16G16B16A16_FLOAT?
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.basic = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.basic->SetName(L"basic");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadVtFeedbackShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/BasicVS.cso", L"../DemoGame/CompiledShaders/VtFeedbackPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = //texturedInputLayout
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_UINT;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtFeedback = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtFeedback->SetName(L"Virtual texture feedback");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadVtFeedbackWithNormalsShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/BasicVS.cso", L"../DemoGame/CompiledShaders/VtFeedbackPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_UINT;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtFeedbackWithNormals = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtFeedbackWithNormals->SetName(L"Virtual texture feedback with normals");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadVtFeedbackWithNormalsTwoSidedShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/BasicVS.cso", L"../DemoGame/CompiledShaders/VtFeedbackPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
				{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_UINT;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtFeedbackWithNormalsTwoSided = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtFeedbackWithNormalsTwoSided->SetName(L"Virtual texture feedback with normals two sided");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadVtDebugDrawShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/TexturedQuadVS.cso", L"../DemoGame/CompiledShaders/VtDebugDrawPs.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = //texturedInputLayout
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.vtDebugDraw = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.vtDebugDraw->SetName(L"virtual texture feedback debug draw");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadCopyShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/PositionOnlyVS.cso", L"../DemoGame/CompiledShaders/CopyPS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = FALSE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.copy = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.copy->SetName(L"Copy");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}

	void loadFireShader(ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, PipelineStateObjects::PipelineLoader* pipelineLoader)
	{
		ShaderRequestVP::create(threadResources, globalResources, L"../DemoGame/CompiledShaders/FireVS.cso", L"../DemoGame/CompiledShaders/FirePS.cso", rootSignature, device, pipelineLoader,
			[](const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				ThreadResources& threadResources, GlobalResources& globalResources, ID3D12RootSignature* rootSignature, ID3D12Device* device, ShaderRequestVP* requests)
		{
			PipelineStateObjects& pipelineStateObjects = globalResources.pipelineStateObjects;
			const D3D12_INPUT_ELEMENT_DESC inputLayout[] = //texturedInputLayout
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
			PSODesc.pRootSignature = rootSignature;
			PSODesc.VS.pShaderBytecode = vertexShaderData;
			PSODesc.VS.BytecodeLength = vertexShaderDataLength;
			PSODesc.PS.pShaderBytecode = pixelShaderData;
			PSODesc.PS.BytecodeLength = pixelShaderDataLength;
			PSODesc.DS.pShaderBytecode = nullptr;
			PSODesc.DS.BytecodeLength = 0u;
			PSODesc.HS.pShaderBytecode = nullptr;
			PSODesc.HS.BytecodeLength = 0u;
			PSODesc.GS.pShaderBytecode = nullptr;
			PSODesc.GS.BytecodeLength = 0u;
			PSODesc.StreamOutput.pSODeclaration = nullptr;
			PSODesc.StreamOutput.NumEntries = 0u;
			PSODesc.StreamOutput.pBufferStrides = nullptr;
			PSODesc.StreamOutput.NumStrides = 0u;
			PSODesc.StreamOutput.RasterizedStream = FALSE;
			PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			PSODesc.BlendState.IndependentBlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
			PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[1].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[1].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[2].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[2].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[3].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[3].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[4].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[4].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[4].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[4].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[4].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[4].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[4].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[5].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[5].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[5].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[5].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[5].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[5].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[5].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[6].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[6].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[6].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[6].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[6].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[6].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[6].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.BlendState.RenderTarget[7].BlendEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].LogicOpEnable = FALSE;
			PSODesc.BlendState.RenderTarget[7].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlend = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
			PSODesc.BlendState.RenderTarget[7].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			PSODesc.BlendState.RenderTarget[7].DestBlendAlpha = D3D12_BLEND_ONE;
			PSODesc.BlendState.RenderTarget[7].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
			PSODesc.BlendState.RenderTarget[7].LogicOp = D3D12_LOGIC_OP_CLEAR;
			PSODesc.BlendState.RenderTarget[7].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			PSODesc.SampleMask = 0xffffffff;
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
			PSODesc.RasterizerState.DepthBias = 0;
			PSODesc.RasterizerState.DepthBiasClamp = 0.0f;
			PSODesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
			PSODesc.RasterizerState.DepthClipEnable = TRUE;
			PSODesc.RasterizerState.MultisampleEnable = FALSE;
			PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PSODesc.RasterizerState.ForcedSampleCount = FALSE;
			PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			PSODesc.DepthStencilState.DepthEnable = TRUE;
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
			PSODesc.DepthStencilState.StencilEnable = FALSE;
			PSODesc.DepthStencilState.StencilReadMask = 0;
			PSODesc.DepthStencilState.StencilWriteMask = 0;
			PSODesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
			PSODesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			PSODesc.InputLayout.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
			PSODesc.InputLayout.pInputElementDescs = inputLayout;
			PSODesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSODesc.NumRenderTargets = 1;
			PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			PSODesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			PSODesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			PSODesc.SampleDesc.Count = 1;
			PSODesc.SampleDesc.Quality = 0;
			PSODesc.NodeMask = 0;
			PSODesc.CachedPSO.pCachedBlob = nullptr;
			PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
			PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

			pipelineStateObjects.fire = D3D12PipelineState(device, PSODesc);
#ifndef NDEBUG
			pipelineStateObjects.fire->SetName(L"fire");
#endif

			requests->componentLoaded(threadResources, globalResources);
		});
	}
}

PipelineStateObjects::PipelineStateObjects(ThreadResources& threadResources, GlobalResources& globalResources, PipelineLoader* pipelineLoader)
{
	ID3D12RootSignature* const rootSignature = globalResources.rootSignatures.rootSignature;
	ID3D12Device* device = globalResources.graphicsEngine.graphicsDevice;

	loadTextShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadDirectionalLightShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadDirectionalLightVtShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadDirectionalLightVtTwoSidedShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadPointLightShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadWaterWithReflectionTextureShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadWaterNoReflectionTextureShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadGlassShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadBasicShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadVtFeedbackShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadVtFeedbackWithNormalsShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadVtFeedbackWithNormalsTwoSidedShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadVtDebugDrawShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadCopyShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
	loadFireShader(threadResources, globalResources, rootSignature, device, pipelineLoader);
}

PipelineStateObjects::~PipelineStateObjects() {}