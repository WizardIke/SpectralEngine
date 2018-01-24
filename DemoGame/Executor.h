#pragma once
#include <BaseExecutor.h>
#include "RenderPass1.h"
#include <array>
class D3D12GraphicsEngine;
class Assets;

class Executor : public BaseExecutor
{
protected:
	void update1(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources);
	virtual void update2(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources) override;
public:
	Executor(SharedResources* const sharedResources, unsigned long uploadHeapStartingSize, unsigned int uploadRequestBufferStartingCapacity, unsigned int halfFinishedUploadRequestBufferStartingCapasity) : 
		BaseExecutor(sharedResources), renderPass(*sharedResources),
		streamingManager(sharedResources->graphicsEngine.graphicsDevice, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
	{}

	static void update1NextPhaseJob(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock);
	static void update2NextPhaseJob(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock) { reinterpret_cast<Executor*>(exe)->update2(std::move(lock), sharedResources); }
	static void initialize(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock);

	StreamingManagerThreadLocal streamingManager;
	RenderPass1::Local renderPass;
};