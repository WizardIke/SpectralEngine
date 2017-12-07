#pragma once
#include <SharedResources.h>
#include <Font.h>
#include "RootSignatures.h"
#include "PipelineStateObjects.h"
#include "UserInterface/UserInterface.h"
#include "Areas.h"
#include "AmbientMusic.h"
#include <../Array/dynamicarray.h>
#include "MainExecutor.h"
#include "PrimaryExecutor.h"
#include "BackgroundExecutor.h"
#include "RenderPass.h"

class Assets : public SharedResources
{
public:
	Assets();

	MainExecutor mainExecutor;
	RootSignatures rootSignatures;
	PipelineStateObjects pipelineStateObjects; //Immutable
	D3D12Resource sharedConstantBuffer;
	RenderPass1 renderPass;
	Font arial; //Immutable
	UserInterface userInterface;
	Areas areas;
	AmbientMusic ambientMusic;
	MainCamera mainCamera;

	DynamicArray<BackgroundExecutor> backgroundExecutors;
	DynamicArray<PrimaryExecutor> primaryExecutors;

	void update(BaseExecutor* const executor);
	void start(BaseExecutor* executor);
};