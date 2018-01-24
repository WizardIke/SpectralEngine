#pragma once

#include <math.h>
#include <DirectXMath.h>
#include "Transform.h"
class BaseExecutor;
class SharedResources;

class PlayerPosition
{
public:
	PlayerPosition(const DirectX::XMFLOAT3 position, const DirectX::XMFLOAT3 rotation);
	~PlayerPosition() {}

	void update(BaseExecutor* const executor, SharedResources& sharedResources);

	float oneOverMass;
	float friction;
	float power;
	float gravity;
	DirectX::XMFLOAT3 velocity;
	Transform location;
	float heightLocked;

	static void mouseMoved(void * const position, float x, float y);
};