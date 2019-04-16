#pragma once

#include <math.h>
#include "Vector3.h"
#include "Transform.h"

class PlayerPosition
{
public:
	PlayerPosition(Vector3 position, Vector3 rotation);
	~PlayerPosition() {}

	void update(float frameTime, bool moveleft, bool moveRight, bool moveForwards, bool moveBackwards, bool moveUp);

	float oneOverMass;
	float friction;
	float power;
	float gravity;
	Vector3 velocity;
	Transform location;
	float heightLocked;

	static void mouseMoved(void* position, float x, float y);
};