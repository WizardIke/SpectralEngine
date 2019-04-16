#pragma once
#include "SinglyLinked.h"

class LinkedTask : public SinglyLinked
{
public:
	void(*execute)(LinkedTask& task, void* tr, void* gr);
	LinkedTask(void(*execute1)(LinkedTask& task, void* tr, void* gr)) : execute(execute1) {}
	LinkedTask() {}
};