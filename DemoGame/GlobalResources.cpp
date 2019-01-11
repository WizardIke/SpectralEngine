#include "GlobalResources.h"
#include "TextureNames.h"

static LRESULT CALLBACK windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCCREATE)
	{
		CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT*>(lParam);
		auto state = create->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
	}
	auto& globalResources = *reinterpret_cast<GlobalResources*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_INPUT:
	{
		globalResources.inputManager.handleRawInput(lParam, globalResources.inputHandler, globalResources);
		return 0;
	}
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			DefWindowProc(hwnd, message, wParam, lParam);
			while (true)
			{
				MSG msg;
				GetMessage(&msg, nullptr, 0, 0);
				hwnd = msg.hwnd;
				message = msg.message;
				wParam = msg.wParam;
				lParam = msg.lParam;
				if (message == WM_CLOSE || message == WM_DESTROY)
				{
					PostQuitMessage(0);
					break;
				}
				else if (message == WM_SYSCOMMAND && ((wParam & 0xFFF0) == SC_MAXIMIZE))
				{
					break;
				}
				DefWindowProc(hwnd, message, wParam, lParam);
			}
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

static void loadingResourceCallback(TextureManager::TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureID)
{
	ThreadResources& threadResources = *reinterpret_cast<ThreadResources*>(tr);
	GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(gr);
	globalResources.arial.setDiffuseTexture(textureID, globalResources.constantBuffersCpuAddress, globalResources.sharedConstantBuffer->GetGPUVirtualAddress());
	globalResources.userInterface.start(threadResources, globalResources);
	globalResources.taskShedular.setNextPhaseTask(ThreadResources::initialize2);

	delete static_cast<TextureManager::TextureStreamingRequest*>(&request);
}

static const wchar_t* const musicFiles[] = 
{
	L"../DemoGame/Music/Heroic_Demise_New.sound",
	L"../DemoGame/Music/Tropic_Strike.sound"
};

GlobalResources::GlobalResources() : GlobalResources(std::thread::hardware_concurrency(), false, false, false) {}

GlobalResources::GlobalResources(const unsigned int numberOfThreads, bool fullScreen, bool vSync, bool enableGpuDebugging) :
	window(this, windowCallback, [fullScreen]() {if (fullScreen) { return GetSystemMetrics(SM_CXVIRTUALSCREEN); } else return GetSystemMetrics(SM_CXSCREEN) / 2; }(),
		[fullScreen]() {if (fullScreen) { return GetSystemMetrics(SM_CYVIRTUALSCREEN); } else return GetSystemMetrics(SM_CYSCREEN) / 2; }(),
		[fullScreen]() {if (fullScreen) { return 0; } else return GetSystemMetrics(SM_CXSCREEN) / 5; }(),
		[fullScreen]() {if (fullScreen) { return 0; } else return GetSystemMetrics(SM_CYSCREEN) / 5; }(), fullScreen, vSync),
	graphicsEngine(window, enableGpuDebugging, /*DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_SOFTWARE*/DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_NONE),
	streamingManager(*graphicsEngine.graphicsDevice, 32u * 1024u * 1024u),
	taskShedular(numberOfThreads > 2u ? numberOfThreads : 2u, ThreadResources::initialize1),
	mainThreadResources(0u, *this, ThreadResources::mainEndUpdate2),
	asynchronousFileManager(),
	textureManager(),
	meshManager(),
	soundEngine(),
	timer(),
	inputManager(),
	inputHandler(window, { PlayerPosition::mouseMoved, &playerPosition }),
	rootSignatures(graphicsEngine.graphicsDevice),
	pipelineStateObjects(graphicsEngine.graphicsDevice, rootSignatures),
	virtualTextureManager(graphicsEngine),
	sharedConstantBuffer(graphicsEngine.graphicsDevice, []()
	{
		D3D12_HEAP_PROPERTIES heapProperties;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.CreationNodeMask = 0u;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.VisibleNodeMask = 0u;
		heapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		return heapProperties;
	}(), D3D12_HEAP_FLAG_NONE, []()
	{
		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.DepthOrArraySize = 1u;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		resourceDesc.Height = 1u;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.MipLevels = 1u;
		resourceDesc.SampleDesc.Count = 1u;
		resourceDesc.SampleDesc.Quality = 0u;
		resourceDesc.Width = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // size of the resource heap. Must be a multiple of 64KB for constant buffers
		return resourceDesc;
	}(), D3D12_RESOURCE_STATE_GENERIC_READ),
	areas(mainThreadResources, *this),
	ambientMusic(*this, musicFiles, sizeof(musicFiles) / sizeof(musicFiles[0])),
	playerPosition(DirectX::XMFLOAT3(59.0f, 4.0f, 10.0f), DirectX::XMFLOAT3(0.0f, 0.2f, 0.0f)),
	warpTexture(graphicsEngine.graphicsDevice, []()
	{
		D3D12_HEAP_PROPERTIES properties;
		properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		properties.VisibleNodeMask = 0u;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.CreationNodeMask = 0u;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		return properties;
	}(), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, window.getBuffer(0u)->GetDesc(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	warpTextureCpuDescriptorHeap(graphicsEngine.graphicsDevice, []()
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;
		return desc;
	}()),
	readyToPresentEvent(nullptr, FALSE, FALSE, nullptr)
{
	ambientMusic.start(mainThreadResources, *this);
	ioCompletionQueue.start(mainThreadResources, *this);

	D3D12_RANGE readRange{ 0u, 0u };
	HRESULT hr = sharedConstantBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&constantBuffersCpuAddress));
	if (FAILED(hr)) throw HresultException(hr);
	unsigned char* cpuConstantBuffer = constantBuffersCpuAddress;
	auto constantBuffersGpuAddress = sharedConstantBuffer->GetGPUVirtualAddress();

	renderPass.~RenderPass1();
	new(&renderPass) RenderPass1(*this, playerPosition.location, window.width(), window.height(), constantBuffersGpuAddress, cpuConstantBuffer, 0.25f * 3.141f);
	mainCamera().init(window, graphicsEngine, window.width(), window.height(), constantBuffersGpuAddress, cpuConstantBuffer, 0.25f * 3.141f, playerPosition.location);
	userInterface.~UserInterface();
	new(&userInterface) UserInterface(*this, constantBuffersGpuAddress, cpuConstantBuffer);
	arial.~Font();
	TextureManager::TextureStreamingRequest* fontTextureRequest = new TextureManager::TextureStreamingRequest(loadingResourceCallback, TextureNames::Arial);
	new(&arial) Font(constantBuffersGpuAddress, cpuConstantBuffer, L"Arial.fnt", mainThreadResources, *this, fontTextureRequest);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1u;
	srvDesc.Texture2D.MostDetailedMip = 0u;
	srvDesc.Texture2D.PlaneSlice = 0u;
	srvDesc.Texture2D.ResourceMinLODClamp = 0u;
	warpTextureDescriptorIndex = graphicsEngine.descriptorAllocator.allocate();
	graphicsEngine.graphicsDevice->CreateShaderResourceView(warpTexture, &srvDesc, graphicsEngine.mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart() +
		graphicsEngine.constantBufferViewAndShaderResourceViewAndUnordedAccessViewDescriptorSize * warpTextureDescriptorIndex);

	D3D12_RENDER_TARGET_VIEW_DESC warpTextureRtvDesc;
	warpTextureRtvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	warpTextureRtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
	warpTextureRtvDesc.Texture2D.MipSlice = 0u;
	warpTextureRtvDesc.Texture2D.PlaneSlice = 0u;
	graphicsEngine.graphicsDevice->CreateRenderTargetView(warpTexture, &warpTextureRtvDesc, warpTextureCpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

#ifndef NDEBUG
	warpTexture->SetName(L"Warp texture");
	warpTextureCpuDescriptorHeap->SetName(L"Warp texture descriptor heap");
#endif
}

GlobalResources::~GlobalResources()
{
	graphicsEngine.descriptorAllocator.deallocate(warpTextureDescriptorIndex);
	mainCamera().destruct(graphicsEngine);
	renderPass.virtualTextureFeedbackSubPass().destruct();
	arial.destruct(textureManager, graphicsEngine, TextureNames::Arial);
}

void GlobalResources::update()
{
	bool succeeded = Window::processMessagesForAllWindowsOnCurrentThread();
	if (!succeeded)
	{
		taskShedular.setNextPhaseTask(ThreadResources::quit);
	}
	timer.update();
	playerPosition.update(timer.frameTime(), inputHandler.aDown, inputHandler.dDown, inputHandler.wDown, inputHandler.sDown, inputHandler.spaceDown);
	mainCamera().update(playerPosition.location);
}

void GlobalResources::beforeRender()
{
	mainCamera().render(window, graphicsEngine);
}

void GlobalResources::start()
{
	const auto threadCount = taskShedular.threadCount();
	const auto primaryThreadCount = threadCount - (threadCount > 4u ? threadCount / 4u : 1u);
	for (unsigned int i = 1u; i != primaryThreadCount; ++i)
	{
		std::thread worker([i, this]()
		{
			ThreadResources threadResources(i, *this, ThreadResources::primaryEndUpdate2);
			threadResources.start(*this);
		});
		worker.detach();
	}
	for (unsigned int i = primaryThreadCount; i != threadCount; ++i)
	{
		std::thread worker([i, this]()
		{
			ThreadResources threadResources(i, *this, ThreadResources::backgroundEndUpdate2);
			threadResources.start(*this);
		});
		worker.detach();
	}
	timer.start();
	mainThreadResources.start(*this);
}