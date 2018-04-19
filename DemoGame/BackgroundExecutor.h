#pragma once
#include "Executor.h"

class BackgroundExecutor : public Executor
{
	std::thread thread;

	virtual void update2(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources) override;
	void getIntoCorrectStateAfterDoingBackgroundJob(SharedResources& sharedResources);
public:
	BackgroundExecutor(SharedResources& sharedResources);
	void run(SharedResources& sharedResources);
};