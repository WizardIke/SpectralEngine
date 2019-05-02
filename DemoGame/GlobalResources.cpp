#include "GlobalResources.h"
#include "TextureNames.h"
#include <thread>

class GlobalResources::Unloader : private PrimaryTaskFromOtherThreadQueue::Task
{
	class AmbientMusicStopRequests : public AmbientMusic::StopRequest
	{
	public:
		Unloader* unloader;

		AmbientMusicStopRequests(Unloader* unloader, void(*callback)(ReadRequest& request, void* tr, void* gr)) :
			unloader(unloader), AmbientMusic::StopRequest(callback) {}
	};

	class UserInterfaceStopRequest : public UserInterface::StopRequest
	{
	public:
		Unloader* unloader;

		UserInterfaceStopRequest(Unloader* unloader, void(*callback)(UserInterface::StopRequest& stopRequest, void* tr, void* gr)) :
			unloader(unloader), UserInterface::StopRequest(callback) {}
	};

	class AreasStopRequest : public Areas::StopRequest
	{
	public:
		Unloader* unloader;

		AreasStopRequest(Unloader* unloader, void(*callback)(Areas::StopRequest& stopRequest, void* tr, void* gr)) :
			unloader(unloader), Areas::StopRequest(callback) {}
	};

	class IoCompletionQueueStopRequest : public RunnableIOCompletionQueue::StopRequest
	{
	public:
		Unloader* unloader;

		IoCompletionQueueStopRequest(Unloader* unloader, void(*callback)(RunnableIOCompletionQueue::StopRequest& stopRequest, void* tr, void* gr)) :
			unloader(unloader), RunnableIOCompletionQueue::StopRequest::StopRequest(callback) {}
	};

	class TaskShedularStopRequest : public TaskShedular<ThreadResources, GlobalResources>::StopRequest
	{
	public:
		Unloader* unloader;

		TaskShedularStopRequest(Unloader* unloader, void(*callback)(TaskShedular<ThreadResources, GlobalResources>::StopRequest& stopRequest, void* tr, void* gr)) :
			unloader(unloader), TaskShedular<ThreadResources, GlobalResources>::StopRequest(callback) {}
	};

	AmbientMusicStopRequests ambientMusicStopRequests;
	UserInterfaceStopRequest userInterfaceStopRequest;
	AreasStopRequest areasStopRequest;

	IoCompletionQueueStopRequest ioCompletionQueueStopRequest;
	TaskShedularStopRequest taskShedularStopRequest;


	constexpr static unsigned int numberOfNumponentsToUnload1 = 3u; //AmbientMusic, UserInterface, Areas
	std::atomic<unsigned int> numberOfComponentsUnloaded = 0u;

	constexpr static unsigned int numberOfNumponentsToUnload2 = 2u; //IoCompletionQueue, TaskShedular

	void componentUnloaded1(void*, void* gr)
	{
		if (numberOfComponentsUnloaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfNumponentsToUnload1 - 1u))
		{
			numberOfComponentsUnloaded.store(0u, std::memory_order_relaxed);

			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr, void* gr)
			{
				Unloader& unloader = static_cast<Unloader&>(task);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

				globalResources.ioCompletionQueue.stop(unloader.ioCompletionQueueStopRequest, threadResources, globalResources);
				globalResources.taskShedular.stop(unloader.taskShedularStopRequest, threadResources, globalResources);
			};
			globalResources.taskShedular.pushPrimaryTaskFromOtherThread(1u, *this);
		}
	}

	void componentUnloaded2(void*, void* gr)
	{
		if (numberOfComponentsUnloaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfNumponentsToUnload2 - 1u))
		{
			delete this;

			GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
			SendMessage(globalResources.window.native_handle(), WM_QUIT, 0, 0);
		}
	}
public:
	Unloader() :
		ambientMusicStopRequests(this, [](AsynchronousFileManager::ReadRequest& stopRequest, void* tr, void* gr)
		{
			static_cast<AmbientMusicStopRequests&>(stopRequest).unloader->componentUnloaded1(tr, gr);
		}),
		userInterfaceStopRequest(this, [](UserInterface::StopRequest& stopRequest, void* tr, void* gr)
		{
			static_cast<UserInterfaceStopRequest&>(stopRequest).unloader->componentUnloaded1(tr, gr);
		}),
		areasStopRequest(this, [](Areas::StopRequest& stopRequest, void* tr, void* gr)
		{
			static_cast<AreasStopRequest&>(stopRequest).unloader->componentUnloaded1(tr, gr);
		}),
		taskShedularStopRequest(this, [](TaskShedular<ThreadResources, GlobalResources>::StopRequest& stopRequest, void* tr, void* gr)
		{
			static_cast<TaskShedularStopRequest&>(stopRequest).unloader->componentUnloaded2(tr, gr);
		}),
		ioCompletionQueueStopRequest(this, [](RunnableIOCompletionQueue::StopRequest& stopRequest, void* tr, void* gr)
		{
			static_cast<IoCompletionQueueStopRequest&>(stopRequest).unloader->componentUnloaded2(tr, gr);
		}) {}

		void unload(ThreadResources& threadResources, GlobalResources& globalReources)
		{
			globalReources.areas.stop(areasStopRequest, threadResources, globalReources);
			globalReources.userInterface.stop(userInterfaceStopRequest, threadResources, globalReources);
			globalReources.ambientMusic.stop(ambientMusicStopRequests, threadResources, globalReources);
		}
};

LRESULT CALLBACK GlobalResources::windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
			while (true)
			{
				DefWindowProc(hwnd, message, wParam, lParam);
				MSG msg;
				GetMessage(&msg, nullptr, 0, 0);
				hwnd = msg.hwnd;
				message = msg.message;
				wParam = msg.wParam;
				lParam = msg.lParam;
				if (message == WM_CLOSE)
				{
					break;
				}
				else if (message == WM_SYSCOMMAND && ((wParam & 0xFFF0) == SC_MAXIMIZE))
				{
					break;
				}
			}
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	case WM_CLOSE:
	{
		if (!globalResources.isQuitiing)
		{
			//The main window is closing so we should unload and quit
			Unloader* unloader = new Unloader();
			unloader->unload(globalResources.mainThreadResources, globalResources);
			globalResources.isQuitiing = true;
		}
		return 0;
	}
	case WM_QUIT:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

class GlobalResources::InitialResourceLoader : public TextureManager::TextureStreamingRequest, public PipelineStateObjects::PipelineLoader, private PrimaryTaskFromOtherThreadQueue::Task
{
	static void fontsLoadedCallback(TextureManager::TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureID)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);
		globalResources.arial.setDiffuseTexture(textureID, globalResources.constantBuffersCpuAddress, globalResources.sharedConstantBuffer->GetGPUVirtualAddress());

		InitialResourceLoader& request1 = static_cast<InitialResourceLoader&>(request);
		request1.componentLoaded(threadResources, globalResources);
		freeRequestMemory(request1);
	}

	static void pipelineStateObjectsLoadedCallback(PipelineStateObjects::PipelineLoader& pipelineLoader, ThreadResources& threadResources, GlobalResources& globalResources)
	{
		InitialResourceLoader& request1 = static_cast<InitialResourceLoader&>(pipelineLoader);
		request1.componentLoaded(threadResources, globalResources);
	}

	void componentLoaded(ThreadResources&, GlobalResources& globalResources)
	{
		if(numberOfComponentsLoaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponentsToLoad - 1u))
		{
			globalResources.taskShedular.setNextPhaseTask(ThreadResources::initialize2);
			execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr, void* gr)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				GlobalResources& globalResources = *static_cast<GlobalResources*>(gr);

				globalResources.userInterface.start(threadResources, globalResources);
				globalResources.areas.start(threadResources, globalResources);

				freeRequestMemory(static_cast<InitialResourceLoader&>(task));
			};
			globalResources.taskShedular.pushPrimaryTaskFromOtherThread(0u, *this);
		}
	}

	static void freeRequestMemory(InitialResourceLoader& request)
	{
		if(request.numberOfComponentsReadyToDelete.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponentsToLoad))
		{
			delete &request;
		}
	}

	static constexpr unsigned int numberOfComponentsToLoad = 2u;
	std::atomic<unsigned int> numberOfComponentsLoaded = 0u;
	std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;
public:
	InitialResourceLoader(const wchar_t* fontFilename) :
		TextureManager::TextureStreamingRequest(fontsLoadedCallback, fontFilename),
		PipelineStateObjects::PipelineLoader(pipelineStateObjectsLoadedCallback, [](PipelineStateObjects::PipelineLoader& pipelineLoader)
	{
		freeRequestMemory(static_cast<InitialResourceLoader&>(pipelineLoader));
	}) {}
};

static const wchar_t* const musicFiles[] = 
{
	L"../DemoGame/Music/Heroic_Demise_New.sound",
	L"../DemoGame/Music/Tropic_Strike.sound"
};

GlobalResources::GlobalResources() : GlobalResources(std::thread::hardware_concurrency(), false, false, false, new InitialResourceLoader(TextureNames::Arial)) {}

GlobalResources::GlobalResources(const unsigned int numberOfThreads, bool fullScreen, bool vSync, bool enableGpuDebugging, void* initialResourceLoader) :
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
	pipelineStateObjects(mainThreadResources, *this, *static_cast<InitialResourceLoader*>(initialResourceLoader)),
	virtualTextureManager(),
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
	areas(),
	ambientMusic(*this, musicFiles, sizeof(musicFiles) / sizeof(musicFiles[0])),
	playerPosition(Vector3(59.0f, 4.0f, 10.0f), Vector3(0.0f, 0.2f, 0.0f)),
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
	areas.setPosition(playerPosition.location.position, 0u);
	taskShedular.start(mainThreadResources, *this);
	ioCompletionQueue.start(mainThreadResources, *this);
	ambientMusic.start(mainThreadResources, *this);

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
	new(&arial) Font(constantBuffersGpuAddress, cpuConstantBuffer, L"Arial.fnt", mainThreadResources, *this, static_cast<InitialResourceLoader*>(initialResourceLoader));

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
		graphicsEngine.cbvAndSrvAndUavDescriptorSize * warpTextureDescriptorIndex);

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
	arial.destruct(mainThreadResources, *this, TextureNames::Arial);
}

void GlobalResources::update()
{
	bool shouldQuit = Window::processMessagesForAllWindowsCreatedOnCurrentThread();
	if (shouldQuit)
	{
		taskShedular.setNextPhaseTask(quit);
	}
	timer.update();
	playerPosition.update(timer.frameTime(), inputHandler.aDown, inputHandler.dDown, inputHandler.wDown, inputHandler.sDown, inputHandler.spaceDown);
	mainCamera().update(playerPosition.location);
}

void GlobalResources::beforeRender()
{
	mainCamera().render(window, graphicsEngine);
}

void GlobalResources::primaryThreadFunction(unsigned int i, unsigned int primaryThreadCount, GlobalResources& globalReources)
{
	std::thread worker = i != primaryThreadCount ? 
		std::thread(primaryThreadFunction, i + 1u, primaryThreadCount, std::ref(globalReources)) : 
		std::thread(backgroundThreadFunction, i + 1u, globalReources.taskShedular.threadCount(), std::ref(globalReources));

	ThreadResources threadResources(i, globalReources, ThreadResources::primaryEndUpdate2);
	threadResources.start(globalReources);

	worker.join();
}

void GlobalResources::backgroundThreadFunction(unsigned int i, unsigned int threadCount, GlobalResources& globalReources)
{
	if(i + 1u != threadCount)
	{
		std::thread worker = std::thread(backgroundThreadFunction, i + 1u, threadCount, std::ref(globalReources));

		ThreadResources threadResources(i, globalReources, ThreadResources::backgroundEndUpdate2);
		threadResources.start(globalReources);

		worker.join();
	}
	else
	{
		ThreadResources threadResources(i, globalReources, ThreadResources::backgroundEndUpdate2);
		threadResources.start(globalReources);
	}
}

void GlobalResources::start()
{
	const auto threadCount = taskShedular.threadCount();
	const auto primaryThreadCount = threadCount - (threadCount > 4u ? threadCount / 4u : 1u) - 1u;

	std::thread worker = primaryThreadCount != 0u ?
		std::thread(primaryThreadFunction, 1u, primaryThreadCount, std::ref(*this)) :
		std::thread(backgroundThreadFunction, 1u, threadCount, std::ref(*this));
	
	timer.start();
	mainThreadResources.start(*this);

	worker.join();

	graphicsEngine.stop();
	streamingManager.stop(mainThreadResources.streamingManager, graphicsEngine.getFrameEvent());
}

void GlobalResources::stop()
{
	SendMessage(window.native_handle(), WM_CLOSE, 0, 0);
}

bool GlobalResources::quit(ThreadResources&, GlobalResources&)
{
	return true;
}