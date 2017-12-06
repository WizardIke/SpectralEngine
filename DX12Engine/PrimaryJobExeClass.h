#pragma once

#include <thread>
#include "BaseExecutor.h"
class SharedResources;

class PrimaryJobExeClassPrivateData : public BaseExecutor
{
	constexpr static size_t uploadHeapStartingSize = 32u * 1024u * 1024u;
	constexpr static unsigned int uploadRequestBufferStartingCapacity = 25u;
	constexpr static unsigned int halfFinishedUploadRequestBufferStartingCapasity = 25u;

	friend class PrimaryJobExeClass;

	PrimaryJobExeClassPrivateData(SharedResources* const sharedResources) : BaseExecutor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
	{
#ifdef _DEBUG
		this->type = "PrimaryJobExeClass";
#endif // _DEBUG
	}
};

class PrimaryJobExeClass : public PrimaryJobExeClassPrivateData
{
	std::thread thread;
public:
	PrimaryJobExeClass(SharedResources* const sharedResources);
	~PrimaryJobExeClass();

	void run();
};