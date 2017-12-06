#pragma once
#include "BaseExecutor.h"
class SharedResources;

class MainThread : public BaseExecutor
{
	constexpr static size_t uploadHeapStartingSize = 32u * 1024u * 1024u;
	constexpr static unsigned int uploadRequestBufferStartingCapacity = 25u;
	constexpr static unsigned int halfFinishedUploadRequestBufferStartingCapasity = 25u;
public: 
	MainThread(SharedResources* const sharedResources);

	virtual void endFrame(std::unique_lock<std::mutex>&& lock) override;
};