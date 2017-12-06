#pragma once

#include <thread>
#include "BaseExecutor.h"

class JobExeClassPrivateData : public BaseExecutor
{
	constexpr static size_t uploadHeapStartingSize = 32u * 1024u * 1024u;
	constexpr static unsigned int uploadRequestBufferStartingCapacity = 25u;
	constexpr static unsigned int halfFinishedUploadRequestBufferStartingCapasity = 25u;
	friend class JobExeClass;
	JobExeClassPrivateData(SharedResources*const sharedResources) : BaseExecutor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
	{
#ifdef _DEBUG
		this->type = "JobExeClass";
#endif // _DEBUG
	}
};

class JobExeClass : public JobExeClassPrivateData
{
	virtual void endFrame(std::unique_lock<std::mutex>&& lock) override;
	void runBackgroundJobs(Job Job);
	void getIntoCorrectStateAfterDoingBackgroundJob(bool wasFirstWorkStealingDecks);
public:
	JobExeClass(SharedResources*const sharedResources);
	void run();
private:
	std::thread thread;
};