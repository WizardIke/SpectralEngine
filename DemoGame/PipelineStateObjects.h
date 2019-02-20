#pragma once
#include <D3D12PipelineState.h>
#include <atomic>
#include <AsynchronousFileManager.h>
#include <Array.h>
class ThreadResources;
class GlobalResources;

class PipelineStateObjects
{
public:
	class ShaderRequestVP
	{
		constexpr static unsigned int numberOfComponents = 2u;

		class RequestHelper : public AsynchronousFileManager::ReadRequest
		{
		public:
			ShaderRequestVP* request;
		};

		ID3D12RootSignature* rootSignature;

		RequestHelper vertexHelper;
		const unsigned char* vertexShaderData;

		RequestHelper pixelHelper;
		const unsigned char* pixelShaderData;

		std::atomic<unsigned int> numberOfComponentsLoaded = 0u;
		std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;
		void(*loadDescription)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
			PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device);
		void(*deleteRequest)(ShaderRequestVP& request);
		void(*loadingFinished)(ShaderRequestVP& request, ThreadResources& threadResources, GlobalResources& globalResources);

		static void freeRequestMemory(AsynchronousFileManager::ReadRequest& request1, void*, void*);
		void shaderFileLoaded(ThreadResources& threadResources, GlobalResources& globalResources);
	public:
		ShaderRequestVP(ThreadResources& threadResources, GlobalResources& globalResources, const wchar_t* vertexFile, const wchar_t* pixelFile, ID3D12RootSignature* rootSignature1,
			void(*loadingFinished1)(ShaderRequestVP& request, ThreadResources& threadResources, GlobalResources& globalResources),
			void(*loadDescription1)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device),
			void(*deleteRequest1)(ShaderRequestVP& request));
	};

	class PipelineLoader;
	class PipelineLoaderImpl
	{
	public:
		class ShaderLoader : public ShaderRequestVP
		{
		public:
			PipelineLoader* pipelineLoader;

			ShaderLoader(ThreadResources& threadResources, GlobalResources& globalResources, const wchar_t* vertexFile, const wchar_t* pixelFile, ID3D12RootSignature* rootSignature1,
				void(*loadingFinished1)(ShaderRequestVP& request, ThreadResources& threadResources, GlobalResources& globalResources),
				void(*loadDescription1)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
					PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device),
				void(*deleteRequest1)(ShaderRequestVP& request), PipelineLoader* pipelineLoader);
		};
	
		struct ShaderInfo
		{
			const wchar_t* vertexShader;
			const wchar_t* pixelShader;
			ID3D12RootSignature* rootSignature;
			void(*loadDescription)(const unsigned char* vertexShaderData, std::size_t vertexShaderDataLength, const unsigned char* pixelShaderData, std::size_t pixelShaderDataLength,
				PipelineStateObjects& pipelineStateObjects, ID3D12RootSignature* rootSignature, ID3D12Device* device);
		};

		constexpr static unsigned int numberOfComponents = 15u;
		std::atomic<unsigned int> numberOfcomponentsLoaded = 0u;
		std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;

		Array<ShaderLoader, numberOfComponents> shaderLoaders;

		static void componentLoaded(ShaderRequestVP& request1, ThreadResources& threadResources, GlobalResources& globalResources);
		static void freeComponent(ShaderRequestVP& request1);

		PipelineLoaderImpl(const ShaderInfo(&shadowInfos)[numberOfComponents], ThreadResources& threadResources, GlobalResources& globalResources, PipelineLoader& pipelineLoader);
	};

	class PipelineLoader
	{
		friend class PipelineLoaderImpl;
		friend class PipelineStateObjects;
		union
		{
			PipelineLoaderImpl impl;
		};
		void(*loadingFinished)(PipelineLoader& pipelineLoader, ThreadResources& threadResources, GlobalResources& globalResources);
		void(*deleteRequest)(PipelineLoader& pipelineLoader);

	public:
		PipelineLoader(void(*loadingFinished)(PipelineLoader& pipelineLoader, ThreadResources& threadResources, GlobalResources& globalResources), void(*deleteRequest)(PipelineLoader& pipelineLoader)) :
			loadingFinished(loadingFinished), deleteRequest(deleteRequest) {}

		~PipelineLoader() {}
	};

	PipelineStateObjects(ThreadResources& threadResources, GlobalResources& globalResources, PipelineLoader& pipelineLoader);
	~PipelineStateObjects() {}

	D3D12PipelineState text;
	D3D12PipelineState directionalLight;
	D3D12PipelineState directionalLightVt;
	D3D12PipelineState directionalLightVtTwoSided;
	D3D12PipelineState pointLight;
	D3D12PipelineState waterWithReflectionTexture;
	D3D12PipelineState waterNoReflectionTexture;
	D3D12PipelineState glass;
	D3D12PipelineState basic;
	D3D12PipelineState fire;
	D3D12PipelineState copy;
	D3D12PipelineState vtFeedback;
	D3D12PipelineState vtFeedbackWithNormals;
	D3D12PipelineState vtFeedbackWithNormalsTwoSided;
	D3D12PipelineState vtDebugDraw;
};