#include "GlobalResources.h"
#include "TextureNames.h"
#include <thread>

#if defined(_WIN32_WINNT) && !defined(NDEBUG)
#include <windows.h>
#include <string>
 
namespace
{
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).  
		DWORD dwThreadID; // Thread ID (-1=caller thread).  
		DWORD dwFlags; // Reserved for future use, must be zero.  
	} THREADNAME_INFO;
#pragma pack(pop)  
	void SetThreadName(DWORD dwThreadID, const char* threadName) {
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
		__try {
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
#pragma warning(pop) 
	}
}
#endif

class GlobalResources::Unloader : private PrimaryTaskFromOtherThreadQueue::Task
{
	class AmbientMusicStopRequests : public AmbientMusic::StopRequest
	{
	public:
		Unloader* unloader;

		AmbientMusicStopRequests(Unloader* unloader, void(*callback)(AmbientMusic::StopRequest& request, void* tr)) :
			unloader(unloader), AmbientMusic::StopRequest(callback) {}
	};

	class UserInterfaceStopRequest : public UserInterface::StopRequest
	{
	public:
		Unloader* unloader;

		UserInterfaceStopRequest(Unloader* unloader, void(*callback)(UserInterface::StopRequest& stopRequest, void* tr)) :
			unloader(unloader), UserInterface::StopRequest(callback) {}
	};

	class AreasStopRequest : public Areas::StopRequest
	{
	public:
		Unloader* unloader;

		AreasStopRequest(Unloader* unloader, void(*callback)(Areas::StopRequest& stopRequest, void* tr)) :
			unloader(unloader), Areas::StopRequest(callback) {}
	};

	class IoCompletionQueueStopRequest : public RunnableIOCompletionQueue::StopRequest
	{
	public:
		Unloader* unloader;

		IoCompletionQueueStopRequest(Unloader* unloader, void(*callback)(RunnableIOCompletionQueue::StopRequest& stopRequest, void* tr)) :
			unloader(unloader), RunnableIOCompletionQueue::StopRequest::StopRequest(callback) {}
	};

	class TaskShedularStopRequest : public TaskShedular<ThreadResources>::StopRequest
	{
	public:
		Unloader* unloader;

		TaskShedularStopRequest(Unloader* unloader, void(*callback)(TaskShedular<ThreadResources>::StopRequest& stopRequest, void* tr)) :
			unloader(unloader), TaskShedular<ThreadResources>::StopRequest(callback) {}
	};

	AmbientMusicStopRequests ambientMusicStopRequests;
	UserInterfaceStopRequest userInterfaceStopRequest;
	AreasStopRequest areasStopRequest;

	IoCompletionQueueStopRequest ioCompletionQueueStopRequest;
	TaskShedularStopRequest taskShedularStopRequest;


	GlobalResources& globalResources;


	constexpr static unsigned int numberOfNumponentsToUnload1 = 3u; //AmbientMusic, UserInterface, Areas
	std::atomic<unsigned int> numberOfComponentsUnloaded = 0u;

	constexpr static unsigned int numberOfNumponentsToUnload2 = 2u; //IoCompletionQueue, TaskShedular

	void componentUnloaded1(void*)
	{
		if (numberOfComponentsUnloaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfNumponentsToUnload1 - 1u))
		{
			numberOfComponentsUnloaded.store(0u, std::memory_order_relaxed);

			execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr)
			{
				Unloader& unloader = static_cast<Unloader&>(task);
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);

				unloader.globalResources.ioCompletionQueue.stop(unloader.ioCompletionQueueStopRequest, threadResources);
				unloader.globalResources.taskShedular.stop(unloader.taskShedularStopRequest, threadResources);
			};
			globalResources.taskShedular.pushPrimaryTaskFromOtherThread(1u, *this);
		}
	}

	void componentUnloaded2(void*)
	{
		if (numberOfComponentsUnloaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfNumponentsToUnload2 - 1u))
		{
			auto& globalResources1 = globalResources;
			delete this;

			auto result = PostMessage(globalResources1.window.native_handle(), WM_QUIT, 0, 0);
			assert(result == TRUE);
		}
	}
public:
	Unloader(GlobalResources& globalResources1) :
		ambientMusicStopRequests(this, [](AmbientMusic::StopRequest& stopRequest, void* tr)
		{
			static_cast<AmbientMusicStopRequests&>(stopRequest).unloader->componentUnloaded1(tr);
		}),
		userInterfaceStopRequest(this, [](UserInterface::StopRequest& stopRequest, void* tr)
		{
			static_cast<UserInterfaceStopRequest&>(stopRequest).unloader->componentUnloaded1(tr);
		}),
		areasStopRequest(this, [](Areas::StopRequest& stopRequest, void* tr)
		{
			static_cast<AreasStopRequest&>(stopRequest).unloader->componentUnloaded1(tr);
		}),
		taskShedularStopRequest(this, [](TaskShedular<ThreadResources>::StopRequest& stopRequest, void* tr)
		{
			static_cast<TaskShedularStopRequest&>(stopRequest).unloader->componentUnloaded2(tr);
		}),
		ioCompletionQueueStopRequest(this, [](RunnableIOCompletionQueue::StopRequest& stopRequest, void* tr)
		{
			static_cast<IoCompletionQueueStopRequest&>(stopRequest).unloader->componentUnloaded2(tr);
		}),
		globalResources(globalResources1)
	{}

		void unload(ThreadResources& threadResources)
		{
			globalResources.areas.stop(areasStopRequest, threadResources);
			globalResources.userInterface.stop(userInterfaceStopRequest, threadResources);
			globalResources.ambientMusic.stop(ambientMusicStopRequests, threadResources);
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
		if (globalResources.isRunning)
		{
			//The main window is closing so we should unload and quit
			Unloader* unloader = new Unloader(globalResources);
			unloader->unload(globalResources.mainThreadResources);
			globalResources.isRunning = false;
		}
		return 0;
	}
	case WM_SIZE:
	{
		auto& window = globalResources.window;
		RECT clientRect;
		GetClientRect(window.native_handle(), &clientRect);
		unsigned int width = clientRect.right - clientRect.left;
		unsigned int height = clientRect.bottom - clientRect.top;
		if ((window.width() != width || window.height() != height) && wParam != SIZE_MINIMIZED)
		{
			globalResources.onResize(width, height);
		}
		if (wParam == SIZE_MAXIMIZED)
		{
			window.onStateChanged(Window::State::maximized);
		}
		else if (wParam == SIZE_RESTORED)
		{
			window.onStateChanged(Window::State::normal);
		}
	}
	case WM_MOVE:
	{
		int xPos = (int)(short)LOWORD(lParam);
		int yPos = (int)(short)HIWORD(lParam);
		auto& window = globalResources.window;
		window.onMove(xPos, yPos);
	}
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

void GlobalResources::onResize(unsigned int width, unsigned int height)
{
	graphicsEngine.waitForGpuIdle();
	window.onResize(width, height);
	graphicsEngine.resize(window);
	refractionRenderer.resize(width, height, graphicsEngine);
	renderPass.resize(width, height, window, graphicsEngine);
}

class GlobalResources::InitialResourceLoader : public TextureManager::TextureStreamingRequest, public PipelineStateObjects::PipelineLoader, private PrimaryTaskFromOtherThreadQueue::Task
{
	static void fontsLoadedCallback(TextureManager::TextureStreamingRequest& request1, void* tr, unsigned int textureID)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		InitialResourceLoader& request = static_cast<InitialResourceLoader&>(request1);

		request.globalResources.arial.setDiffuseTexture(textureID, request.globalResources.constantBuffersCpuAddress, request.globalResources.sharedConstantBuffer->GetGPUVirtualAddress());

		
		request.componentLoaded(threadResources);
	}

	static void pipelineStateObjectsLoadedCallback(PipelineStateObjects::PipelineLoader& pipelineLoader, void* tr)
	{
		ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
		InitialResourceLoader& request1 = static_cast<InitialResourceLoader&>(pipelineLoader);
		request1.componentLoaded(threadResources);
	}

	void componentLoaded(ThreadResources&)
	{
		if(numberOfComponentsLoaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponentsToLoad - 1u))
		{
			execute = [](PrimaryTaskFromOtherThreadQueue::Task& task, void* tr)
			{
				ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
				InitialResourceLoader& request = static_cast<InitialResourceLoader&>(task);

				request.globalResources.isRunning = true;

				request.globalResources.userInterface.start(threadResources);
				request.globalResources.areas.start(threadResources);

				delete &request;
			};
			globalResources.taskShedular.pushPrimaryTaskFromOtherThread(0u, *this);
		}
	}

	static constexpr unsigned int numberOfComponentsToLoad = 2u;
	std::atomic<unsigned int> numberOfComponentsLoaded = 0u;
	GlobalResources& globalResources;
public:
	InitialResourceLoader(GlobalResources& globalResources, const wchar_t* fontFilename) :
		TextureManager::TextureStreamingRequest(fontsLoadedCallback, fontFilename),
		PipelineStateObjects::PipelineLoader(pipelineStateObjectsLoadedCallback),
		globalResources(globalResources)
	{}
};

static const wchar_t* const musicFiles[] = 
{
	L"../DemoGame/Music/Heroic_Demise_New.sound",
	L"../DemoGame/Music/Tropic_Strike.sound"
};

GlobalResources::GlobalResources() : GlobalResources(std::thread::hardware_concurrency(),
	Window::State::fullscreen, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN), GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
	false, false, new InitialResourceLoader(*this, TextureNames::Arial)) {}

GlobalResources::GlobalResources(const unsigned int numberOfThreads, Window::State windowState, int screenWidth, int screenHeight, int screenPositionX, int screenPositionY,
	bool vSync, bool enableGpuDebugging, void* initialResourceLoader
#ifndef NDEBUG
	, bool isWarp
#endif
) :
	isRunning(false),
	window(this, windowCallback, (unsigned int)screenWidth, (unsigned int)screenHeight, screenPositionX, screenPositionY,
		((unsigned int)screenWidth) / 2u, ((unsigned int)screenHeight) / 2u, screenPositionX + screenWidth / 4, screenPositionY + screenHeight / 5,
		windowState, vSync),
	graphicsEngine(window, enableGpuDebugging, 
#ifndef NDEBUG
		isWarp,
#endif
		/*DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_SOFTWARE*/DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_NONE),
	streamingManager(*graphicsEngine.graphicsDevice, 32u * 1024u * 1024u),
	taskShedular(numberOfThreads > 2u ? numberOfThreads : 2u, ThreadResources::endUpdate1),
	mainThreadResources(0u, *this, ThreadResources::mainEndUpdate2),
	ioCompletionQueue(),
	asynchronousFileManager(ioCompletionQueue),
	textureManager(asynchronousFileManager, streamingManager, graphicsEngine),
	meshManager(asynchronousFileManager, streamingManager, *graphicsEngine.graphicsDevice),
	soundEngine(),
	timer(),
	inputManager(),
	inputHandler(window, { PlayerPosition::mouseMoved, &playerPosition }),
	rootSignatures(graphicsEngine.graphicsDevice),
	pipelineStateObjects(asynchronousFileManager, *graphicsEngine.graphicsDevice, rootSignatures, *static_cast<InitialResourceLoader*>(initialResourceLoader)
#ifndef NDEBUG
		, isWarp
#endif
	),
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
	renderPass(taskShedular, streamingManager, graphicsEngine, asynchronousFileManager, window.width(), window.height(), playerPosition.location, 0.25f * 3.141f, window),
	virtualTextureManager(renderPass.virtualTextureFeedbackSubPass().pageProvider, streamingManager, graphicsEngine, asynchronousFileManager),
	arial(L"Arial.fnt", mainThreadResources, textureManager, (float)window.width() / (float)window.height(), *static_cast<InitialResourceLoader*>(initialResourceLoader)),
	userInterface(*this),
	areas(this),
	ambientMusic(soundEngine, asynchronousFileManager, musicFiles, sizeof(musicFiles) / sizeof(musicFiles[0])),
	playerPosition(Vector3(59.0f, 4.0f, 10.0f), Vector3(0.0f, 0.2f, 0.0f)),
	refractionRenderer(window.width(), window.height(), graphicsEngine),
	readyToPresentEvent(nullptr, FALSE, FALSE, nullptr)
{
	areas.setPosition(playerPosition.location.position, 0u);
	taskShedular.start(mainThreadResources);
	ioCompletionQueue.start(mainThreadResources);
	ambientMusic.start(mainThreadResources);

	D3D12_RANGE readRange{ 0u, 0u };
	HRESULT hr = sharedConstantBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&constantBuffersCpuAddress));
	if (FAILED(hr)) throw HresultException(hr);
	unsigned char* cpuConstantBuffer = constantBuffersCpuAddress;
	auto constantBuffersGpuAddress = sharedConstantBuffer->GetGPUVirtualAddress();

	renderPass.setConstantBuffers(constantBuffersGpuAddress, cpuConstantBuffer);
	mainCamera().setConstantBuffers(constantBuffersGpuAddress, cpuConstantBuffer);
	userInterface.setConstantBuffers(constantBuffersGpuAddress, cpuConstantBuffer);
	arial.setConstantBuffers(constantBuffersGpuAddress, cpuConstantBuffer);

	window.setForgroundAndShow();
}

GlobalResources::~GlobalResources()
{
	refractionRenderer.destruct(graphicsEngine);
	mainCamera().destruct(graphicsEngine);
	arial.destruct(mainThreadResources, textureManager, TextureNames::Arial);
}

bool GlobalResources::update()
{
	bool shouldQuit = Window::processMessagesForAllWindowsCreatedOnCurrentThread();
	if (shouldQuit)
	{
		return true;
	}
	timer.update();
	playerPosition.update(timer.frameTime(), inputHandler.aDown, inputHandler.dDown, inputHandler.wDown, inputHandler.sDown, inputHandler.spaceDown);

	return false;
}

void GlobalResources::beforeRender()
{
	mainCamera().beforeRender(window, graphicsEngine);
}

void GlobalResources::primaryThreadFunction(unsigned int i, unsigned int primaryThreadCount, GlobalResources& globalReources)
{
#if defined(_WIN32_WINNT) && !defined(NDEBUG)
	std::string threadName = "Primary thread ";
	threadName += std::to_string(i);
	SetThreadName((DWORD)-1, threadName.c_str());
#endif
	std::thread worker = i != primaryThreadCount ? 
		std::thread(primaryThreadFunction, i + 1u, primaryThreadCount, std::ref(globalReources)) : 
		std::thread(backgroundThreadFunction, i + 1u, globalReources.taskShedular.threadCount(), std::ref(globalReources));

	ThreadResources threadResources(i, globalReources, ThreadResources::primaryEndUpdate2);
	threadResources.startPrimary(globalReources);

	worker.join();
}

void GlobalResources::backgroundThreadFunction(unsigned int i, unsigned int threadCount, GlobalResources& globalReources)
{
#if defined(_WIN32_WINNT) && !defined(NDEBUG)
	std::string threadName = "Background thread ";
	threadName += std::to_string(i);
	SetThreadName((DWORD)-1, threadName.c_str());
#endif
	std::thread worker = i + 1u != threadCount ? std::thread(backgroundThreadFunction, i + 1u, threadCount, std::ref(globalReources)) : std::thread();

	ThreadResources threadResources(i, globalReources, ThreadResources::backgroundEndUpdate2);
	threadResources.startBackground(globalReources);

	if (worker.joinable())
	{
		worker.join();
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
	mainThreadResources.startPrimary(*this);

	worker.join();
}

void GlobalResources::stop()
{
	SendMessage(window.native_handle(), WM_CLOSE, 0, 0);
}

bool GlobalResources::quit(ThreadResources& threadResources, void* context)
{
	auto& globalResources = *static_cast<GlobalResources*>(context);
	threadResources.taskShedular.stop(globalResources.taskShedular, [&globalResources, &threadResources]()
		{
			//flush the command queues so command allocators and other gpu resources can be freed.
			globalResources.graphicsEngine.waitForGpuIdle();
			globalResources.streamingManager.stop(threadResources.streamingManager, globalResources.graphicsEngine.getFrameEvent());
		});
	return true;
}