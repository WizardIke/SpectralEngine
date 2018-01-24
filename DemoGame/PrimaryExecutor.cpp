#include "PrimaryExecutor.h"

PrimaryExecutor::PrimaryExecutor(SharedResources& sharedResources) : Executor(&sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
{
#ifdef _DEBUG
	type = "PrimaryExecutor";
#endif // _DEBUG
}

PrimaryExecutor::~PrimaryExecutor() {}

void PrimaryExecutor::run(SharedResources& sharedResources)
{
	thread = std::thread([](std::pair<Executor*, SharedResources*> data) {data.first->run(*data.second); }, std::make_pair(static_cast<Executor*const>(this), &sharedResources));
}