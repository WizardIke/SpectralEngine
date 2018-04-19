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
	Executor(SharedResources& sharedResources) : 
		BaseExecutor(sharedResources), renderPass(sharedResources)
	{}

	static void update1NextPhaseJob(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock);
	static void update2NextPhaseJob(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock) { reinterpret_cast<Executor*>(exe)->update2(std::move(lock), sharedResources); }
	static void initialize(BaseExecutor* exe, SharedResources& sharedResources, std::unique_lock<std::mutex>&& lock);

	RenderPass1::Local renderPass;
};