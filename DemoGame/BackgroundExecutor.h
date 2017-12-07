#pragma once
#include "Executor.h"

class BackgroundExecutor : public Executor
{
	constexpr static size_t uploadHeapStartingSize = 32u * 1024u * 1024u;
	constexpr static unsigned int uploadRequestBufferStartingCapacity = 25u;
	constexpr static unsigned int halfFinishedUploadRequestBufferStartingCapasity = 25u;
	std::thread thread;

	virtual void update2(std::unique_lock<std::mutex>&& lock) override;
	void getIntoCorrectStateAfterDoingBackgroundJob();
public:
	BackgroundExecutor(SharedResources*const sharedResources);
	void run();
};