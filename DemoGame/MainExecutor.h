#pragma once
#include "Executor.h"

class MainExecutor : public Executor
{
	constexpr static size_t uploadHeapStartingSize = 32u * 1024u * 1024u;
	constexpr static unsigned int uploadRequestBufferStartingCapacity = 25u;
	constexpr static unsigned int halfFinishedUploadRequestBufferStartingCapasity = 25u;
public:
	MainExecutor(SharedResources* const sharedResources);

	virtual void update2(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources) override;
};