#pragma once

#include "Sentences/CPUUsageSentence.h"
#include "Sentences/FPSSentence.h"
class Executor;
class SharedResources;

class UserInterface
{
	CPUUsageSentence CPUUsageSentence;
	FPSSentence FPSSentence;

	void update1(Executor* const executor);
	void update2(Executor* const executor);

	void restart(Executor* const executor);
public:
	UserInterface(Executor* const executor);
	~UserInterface() {}

	void start(Executor* const executor);
};