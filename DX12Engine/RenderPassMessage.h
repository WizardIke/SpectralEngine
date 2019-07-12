#pragma once
#include "SinglyLinked.h"

class RenderPassMessage : public SinglyLinked
{
public:
	void(*execute)(RenderPassMessage& message, void* tr);

	RenderPassMessage(void(*execute1)(RenderPassMessage& message, void* tr)) : execute(execute1) {}
};