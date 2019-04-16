#pragma once
#include "Vector3.h"
#include "World.h"

class Portal
{
public:
	Vector3 position;
	Vector3 exitPosition;
	unsigned long exitWorldIndex;

	Portal(Vector3 position1, Vector3 exitPosition1, unsigned long exitWorldIndex1) : position(std::move(position1)), exitPosition(std::move(exitPosition1)), exitWorldIndex(exitWorldIndex1) {}
};