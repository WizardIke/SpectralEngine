#include "PrimaryJobExeClass.h"

PrimaryJobExeClass::PrimaryJobExeClass(SharedResources* const sharedResources) : PrimaryJobExeClassPrivateData(sharedResources),
	thread() {}

PrimaryJobExeClass::~PrimaryJobExeClass() {}

void PrimaryJobExeClass::run()
{
	thread = std::thread(PrimaryJobExeClass::mRun, static_cast<BaseExecutor*const>(this));
}