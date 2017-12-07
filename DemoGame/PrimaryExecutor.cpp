#include "PrimaryExecutor.h"

PrimaryExecutor::PrimaryExecutor(SharedResources* const sharedResources) : Executor(sharedResources, uploadHeapStartingSize, uploadRequestBufferStartingCapacity, halfFinishedUploadRequestBufferStartingCapasity)
{
#ifdef _DEBUG
	type = "PrimaryExecutor";
#endif // _DEBUG
}

PrimaryExecutor::~PrimaryExecutor() {}

void PrimaryExecutor::run()
{
	thread = std::thread([](Executor*const executor) {executor->run(); }, static_cast<Executor*const>(this));
}