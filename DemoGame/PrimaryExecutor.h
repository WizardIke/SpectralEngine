#pragma once
#include "Executor.h"

class PrimaryExecutor : public Executor
{
	std::thread thread;
public:
	PrimaryExecutor(SharedResources& sharedResources);
	~PrimaryExecutor();
	void run(SharedResources& sharedResources);
};