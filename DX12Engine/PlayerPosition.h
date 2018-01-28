#pragma once

#include <math.h>
#include <DirectXMath.h>
#include "Transform.h"
class BaseExecutor;
class SharedResources;

class PlayerPosition
{
	void updateImpl(BaseExecutor* const executor, SharedResources& sharedResources, bool moveleft, bool moveRight, bool moveForwards, bool moveBackwards, bool moveUp);
public:
	PlayerPosition(const DirectX::XMFLOAT3 position, const DirectX::XMFLOAT3 rotation);
	~PlayerPosition() {}

	template<class SharedResources_t>
	void update(BaseExecutor* const executor, SharedResources_t& sharedResources)
	{
		auto& inputHandler = sharedResources.inputHandler;
		updateImpl(executor, sharedResources, inputHandler.aDown, inputHandler.dDown, inputHandler.wDown, inputHandler.sDown, inputHandler.spaceDown);
	}

	float oneOverMass;
	float friction;
	float power;
	float gravity;
	DirectX::XMFLOAT3 velocity;
	Transform location;
	float heightLocked;

	static void mouseMoved(void * const position, float x, float y);
};