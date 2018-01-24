#pragma once
#include "Executor.h"

class BackgroundExecutor : public Executor
{
	constexpr static size_t uploadHeapStartingSize = 32u * 1024u * 1024u;
	constexpr static unsigned int uploadRequestBufferStartingCapacity = 25u;
	constexpr static unsigned int halfFinishedUploadRequestBufferStartingCapasity = 25u;
	std::thread thread;

	virtual void update2(std::unique_lock<std::mutex>&& lock, SharedResources& sharedResources) override;
	void getIntoCorrectStateAfterDoingBackgroundJob(SharedResources& sharedResources);
public:
	BackgroundExecutor(SharedResources& sharedResources);
	void run(SharedResources& sharedResources);
};