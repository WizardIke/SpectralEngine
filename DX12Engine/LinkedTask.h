#pragma once
#include "SinglyLinked.h"

class LinkedTask : public SinglyLinked
{
public:
	void(*execute)(LinkedTask& task, void* tr);
	LinkedTask(void(*execute1)(LinkedTask& task, void* tr)) noexcept : execute(execute1) {}
	LinkedTask(LinkedTask&&) noexcept = default;
	LinkedTask(const LinkedTask&) noexcept = default;
	LinkedTask() noexcept {}
};