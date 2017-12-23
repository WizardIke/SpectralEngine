#pragma once
#include <BaseExecutor.h>
#include "RenderPass1.h"
#include <array>
class D3D12GraphicsEngine;

class Executor : public BaseExecutor
{
protected:
	void update1(std::unique_lock<std::mutex>&& lock);
	virtual void update2(std::unique_lock<std::mutex>&& lock) override;
public:
	Executor(SharedResources* const sharedResources, unsigned long uploadHeapStartingSize, unsigned int uploadRequestBufferStartingCapacity, unsigned int halfFinishedUploadRequestBufferStartingCapasity) : 
		BaseExecutor(sharedResources), renderPass(this),
		streamingManager(sharedResources->graphicsEngine.graphicsDevice, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
	{}

	static void update1NextPhaseJob(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock);
	static void update2NextPhaseJob(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock) { reinterpret_cast<Executor*>(exe)->update2(std::move(lock)); }
	static void initialize(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock) 
	{
		auto executor = reinterpret_cast<Executor*>(exe);
		exe->initialize<Executor::update1NextPhaseJob>(std::move(lock), executor->streamingManager);
	}

	StreamingManagerThreadLocal streamingManager;//local
	RenderPass1::Local renderPass;

	uint32_t frameIndex() { return sharedResources->graphicsEngine.frameIndex; }
};