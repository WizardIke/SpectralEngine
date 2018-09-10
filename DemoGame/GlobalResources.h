#pragma once
#include <TaskShedular.h>
#include "ThreadResources.h"
#include <thread>
#include <Window.h>
#include <D3D12GraphicsEngine.h>
#include <StreamingManager.h>
#include <StartableIOCompletionQueue.h>
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
class ThreadResources;

class GlobalResources
{
	GlobalResources(unsigned int numberOfThreads, bool fullScreen, bool vSync, bool enableGpuDebugging);
public:
	Window window;
	D3D12GraphicsEngine graphicsEngine;
	StreamingManager streamingManager; //thread safe
	TaskShedular<ThreadResources, GlobalResources> taskShedular;
	ThreadResources mainThreadResources;
	StartableIOCompletionQueue ioCompletionQueue;
	AsynchronousFileManager asynchronousFileManager;
	TextureManager textureManager; //thread safe
	MeshManager meshManager; //thread safe
	SoundEngine soundEngine;
	Timer timer;
	InputManager inputManager;
	InputHandler inputHandler;
	RootSignatures rootSignatures;
	PipelineStateObjects pipelineStateObjects; //Immutable
	VirtualTextureManager virtualTextureManager;
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

	GlobalResources();
	~GlobalResources();

	MainCamera& mainCamera()
	{
		return *renderPass.colorSubPass().cameras().begin();
	}
	void update(ThreadResources& threadResources);
	void start();
};