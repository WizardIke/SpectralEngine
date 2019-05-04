#pragma once
#include <TaskShedular.h>
#include "ThreadResources.h"
#include <Window.h>
#include <GraphicsEngine.h>
#include <StreamingManager.h>
#include <RunnableIOCompletionQueue.h>
#include <AsynchronousFileManager.h>
#include <TextureManager.h>
#include <MeshManager.h>
#include <SoundEngine.h>
#include <Timer.h>
#include <InputManager.h>
#include "InputHander.h"
#include "RootSignatures.h"
#include "PipelineStateObjects.h"
#include <VirtualTextureManager.h>
#include <D3D12Resource.h>
#include "RenderPass1.h"
#include <Font.h>
#include "UserInterface/UserInterface.h"
#include "Areas.h"
#include "AmbientMusic.h"
#include <PlayerPosition.h>
#include <D3D12DescriptorHeap.h>
#include <Win32Event.h>
#include <atomic>
#include <EntityGenerator.h>

class GlobalResources
{
	friend class ThreadResources;
	class InitialResourceLoader;
	friend class GlobalResources::InitialResourceLoader;
	class Unloader;
	friend class Unloader;

	GlobalResources(unsigned int numberOfThreads, bool fullScreen, bool vSync, bool enableGpuDebugging, void*);

	/*
	Can only be called on main thread
	*/
	void update();

	void beforeRender();

	static void primaryThreadFunction(unsigned int i, unsigned int primaryThreadCount, GlobalResources& globalReources);
	static void backgroundThreadFunction(unsigned int i, unsigned int threadCount, GlobalResources& globalReources);

	static LRESULT CALLBACK windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/*
	Causes the program to stop running.
	*/
	static bool quit(ThreadResources& threadResources, GlobalResources& globalResources);
	bool isRunning;
public:
	Window window; //must only be used on main thread
	GraphicsEngine graphicsEngine;
	StreamingManager streamingManager; //thread safe
	TaskShedular<ThreadResources, GlobalResources> taskShedular;
	ThreadResources mainThreadResources;
	RunnableIOCompletionQueue ioCompletionQueue;
	AsynchronousFileManager asynchronousFileManager; //thread safe
	TextureManager textureManager; //thread safe
	MeshManager meshManager; //thread safe
	SoundEngine soundEngine;
	Timer timer;
	InputManager inputManager;
	InputHandler inputHandler;
	RootSignatures rootSignatures;
	PipelineStateObjects pipelineStateObjects; //Immutable
	VirtualTextureManager virtualTextureManager; //thread safe
	D3D12Resource sharedConstantBuffer;
	unsigned char* constantBuffersCpuAddress;
	RenderPass1 renderPass;
	Font arial; //Immutable
	UserInterface userInterface;
	Areas areas;
	AmbientMusic ambientMusic;
	PlayerPosition playerPosition;
	D3D12Resource warpTexture;
	unsigned int warpTextureDescriptorIndex;
	D3D12DescriptorHeap warpTextureCpuDescriptorHeap;
	Event readyToPresentEvent;
	std::atomic<unsigned int> readyToPresentCount = 0u;
	EntityGenerator entityGenerator;

	MainCamera& mainCamera()
	{
		return *renderPass.colorSubPass().cameras().begin();
	}

	GlobalResources();

	/*
	Must be called on the main thread after all other threads have stopped using GlobalResources
	*/
	~GlobalResources();

	void start();
	void stop();
};