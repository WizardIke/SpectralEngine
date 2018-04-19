#pragma once
#include "Executor.h"

class MainExecutor : public Executor
{
public:
	MainExecutor(SharedResources& sharedResources);

	virtual void update2(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources) override;
};