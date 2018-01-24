#pragma once

#include "Sentences/CPUUsageSentence.h"
#include "Sentences/FPSSentence.h"
class Executor;
class SharedResources;

class UserInterface
{
	CPUUsageSentence CPUUsageSentence;
	FPSSentence FPSSentence;

	void update1(SharedResources& sharedResources);
	void update2(Executor* const executor, SharedResources& sharedResources);

	void restart(Executor* const executor);
public:
	UserInterface(SharedResources& sharedResources);
	~UserInterface() {}

	void start(Executor* const executor, SharedResources& sharedResources);
};