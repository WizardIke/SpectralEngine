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
		BaseExecutor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity), renderPass(this) {}

	static void update1NextPhaseJob(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock);
	static void update2NextPhaseJob(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock) { reinterpret_cast<Executor*>(exe)->update2(std::move(lock)); }
	static void initialize(BaseExecutor* exe, std::unique_lock<std::mutex>&& lock) 
	{
		exe->initialize<Executor::update1NextPhaseJob>(std::move(lock)); 
	}

	RenderPass1::Local renderPass;

	uint32_t frameIndex() { return sharedResources->graphicsEngine.frameIndex; }
};