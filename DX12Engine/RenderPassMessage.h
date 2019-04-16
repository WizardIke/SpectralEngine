#pragma once
#include "SinglyLinked.h"

class RenderPassMessage : public SinglyLinked
{
public:
	void(*execute)(RenderPassMessage& message, void* tr, void* gr);

	RenderPassMessage(void(*execute1)(RenderPassMessage& message, void* tr, void* gr)) : execute(execute1) {}
};