#pragma once
#include "Executor.h"

class PrimaryExecutor : public Executor
{
	constexpr static size_t uploadHeapStartingSize = 32u * 1024u * 1024u;
	constexpr static unsigned int uploadRequestBufferStartingCapacity = 25u;
	constexpr static unsigned int halfFinishedUploadRequestBufferStartingCapasity = 25u;
	std::thread thread;
public:
	PrimaryExecutor(SharedResources& sharedResources);
	~PrimaryExecutor();
	void run(SharedResources& sharedResources);
};