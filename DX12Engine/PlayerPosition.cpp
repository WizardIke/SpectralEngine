#include "PlayerPosition.h"
#include "InputManager.h"
#include "SharedResources.h"
#include "BaseExecutor.h"

PlayerPosition::PlayerPosition(const DirectX::XMFLOAT3 position, const DirectX::XMFLOAT3 rotation) : 
	location{ position, rotation },
	heightLocked(false),
	velocity{ 0.0f, 0.0f, 0.0f },
	power(50.0f),
	oneOverMass(1.0f / 70.0f)
{
	gravity = 9.8f;
	friction = 50.0f;
}

void PlayerPosition::mouseMoved(void * const position, float x, float y)
{
	auto ths = reinterpret_cast<PlayerPosition*>(position);
	ths->location.rotation.y += static_cast<float>(x);
	if (ths->location.rotation.y > 3.14159265359f) ths->location.rotation.y -= 2.0f * 3.14159265359f;
	else if (ths->location.rotation.y < -3.14159265359f) ths->location.rotation.y += 2.0f * 3.14159265359f;

	ths->location.rotation.x += static_cast<float>(y);
	if (ths->location.rotation.x > 0.5f * 3.14159265359f) ths->location.rotation.x = 0.5f * 3.14159265359f;
	else if (ths->location.rotation.x < -0.5f * 3.14159265359f) ths->location.rotation.x = -0.5f * 3.14159265359f;
}

void PlayerPosition::updateImpl(BaseExecutor* const executor, SharedResources& sharedResources, bool moveleft, bool moveRight, bool moveForwards, bool moveBackwards, bool moveUp)
{
	DirectX::XMVECTOR lookUpVector, lookAtVector;
	DirectX::XMMATRIX rotationMatrix;
	rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(location.rotation.x, location.rotation.y, location.rotation.z);
	DirectX::XMFLOAT3 temp = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
	lookAtVector = DirectX::XMVector3TransformCoord(XMLoadFloat3(&temp), rotationMatrix);
	DirectX::XMFLOAT3 temp2 = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	lookUpVector = DirectX::XMVector3TransformCoord(XMLoadFloat3(&temp2), rotationMatrix);
	DirectX::XMVECTOR LookRightVector = DirectX::XMVector3Cross(lookUpVector, lookAtVector);
	DirectX::XMFLOAT3 LookUp, LookRight, LookAt;
	DirectX::XMStoreFloat3(&LookUp, lookUpVector);
	DirectX::XMStoreFloat3(&LookRight, LookRightVector);
	DirectX::XMStoreFloat3(&LookAt, lookAtVector);
	
	float forceX, forceY, forceZ;
	forceX = forceY = forceZ = 0.0f;
	float speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);

	if (velocity.x > 0.0f)
	{
		velocity.x -= friction * velocity.x / speed;
		if (velocity.x < 0.0f) velocity.x = 0.0f;
	}
	else if (velocity.x < 0.0f)
	{
		velocity.x -= friction * velocity.x / speed;
		if (velocity.x > 0.0f) velocity.x = 0.0f;
	}
	if (velocity.y > 0.0f)
	{
		velocity.y -= friction * velocity.y / speed;
		if (velocity.y < 0.0f) velocity.y = 0.0f;
	}
	else if (velocity.y < 0.0f)
	{
		velocity.y -= friction * velocity.y / speed;
		if (velocity.y > 0.0f) velocity.y = 0.0f;
	}
	if (velocity.z > 0.0f)
	{
		velocity.z -= friction * velocity.z / speed;
		if (velocity.z < 0.0f) velocity.z = 0.0f;
	}
	else if (velocity.z < 0.0f)
	{
		velocity.z -= friction * velocity.z / speed;
		if (velocity.z > 0.0f) velocity.z = 0.0f;
	}

	if (heightLocked)
	{
		float AtLength = sqrt(LookAt.x * LookAt.x + LookAt.z * LookAt.z);
		float RightLength = sqrt(LookAt.x * LookAt.x + LookAt.z * LookAt.z);
		if (moveForwards && moveleft)
		{
			forceX = (LookAt.x * power / (velocity.x + 0.01f) / AtLength - LookRight.x * power / (velocity.x + 0.01f) / RightLength) * 0.707106781186547f;
			forceY = (LookAt.y * power / (velocity.y + 0.01f) / AtLength - LookRight.y * power / (velocity.y + 0.01f) / RightLength) * 0.707106781186547f;
			forceZ = (LookAt.z * power / (velocity.z + 0.01f) / AtLength - LookRight.z * power / (velocity.z + 0.01f) / RightLength) * 0.707106781186547f;
		}
		else if (moveForwards && moveRight)
		{
			forceX = (LookAt.x * power / (velocity.x + 0.01f) / AtLength + LookRight.x * power / (velocity.x + 0.01f) / RightLength) * 0.707106781186547f;
			forceY = (LookAt.y * power / (velocity.y + 0.01f) / AtLength + LookRight.y * power / (velocity.y + 0.01f) / RightLength) * 0.707106781186547f;
			forceZ = (LookAt.z * power / (velocity.z + 0.01f) / AtLength + LookRight.z * power / (velocity.z + 0.01f) / RightLength) * 0.707106781186547f;
		}
		else if (moveBackwards && moveleft)
		{
			forceX = (-LookAt.x * power / (velocity.x + 0.01f) / AtLength - LookRight.x * power / (velocity.x + 0.01f) / RightLength) * 0.707106781186547f;
			forceY = (-LookAt.y * power / (velocity.y + 0.01f) / AtLength - LookRight.y * power / (velocity.y + 0.01f) / RightLength) * 0.707106781186547f;
			forceZ = (-LookAt.z * power / (velocity.z + 0.01f) / AtLength - LookRight.z * power / (velocity.z + 0.01f) / RightLength) * 0.707106781186547f;
		}
		else if (moveBackwards && moveRight)
		{
			forceX = (-LookAt.x * power / (velocity.x + 0.01f) / AtLength + LookRight.x * power / (velocity.x + 0.01f) / RightLength) * 0.707106781186547f;
			forceY = (-LookAt.y * power / (velocity.y + 0.01f) / AtLength + LookRight.y * power / (velocity.y + 0.01f) / RightLength) * 0.707106781186547f;
			forceZ = (-LookAt.z * power / (velocity.z + 0.01f) / AtLength + LookRight.z * power / (velocity.z + 0.01f) / RightLength) * 0.707106781186547f;
		}
		if (moveForwards)
		{
			forceX = LookAt.x * power / (velocity.x + 0.01f) / AtLength;
			forceY = LookAt.y * power / (velocity.y + 0.01f) / AtLength;
			forceZ = LookAt.z * power / (velocity.z + 0.01f) / AtLength;
		}
		if (moveBackwards)
		{
			forceX = -LookAt.x * power / (velocity.x + 0.01f) / AtLength;
			forceY = -LookAt.y * power / (velocity.y + 0.01f) / AtLength;
			forceZ = -LookAt.z * power / (velocity.z + 0.01f) / AtLength;
		}
		if (moveRight)
		{
			forceX = LookRight.x * power / (velocity.x + 0.01f) / AtLength;
			forceY = LookRight.y * power / (velocity.y + 0.01f) / AtLength;
			forceZ = LookRight.z * power / (velocity.z + 0.01f) / AtLength;
		}
		if (moveleft)
		{
			forceX = -LookRight.x * power / (velocity.x + 0.01f) / AtLength;
			forceY = -LookRight.y * power / (velocity.y + 0.01f) / AtLength;
			forceZ = -LookRight.z * power / (velocity.z + 0.01f) / AtLength;
		}
	}
	else
	{
		if (moveForwards && moveleft)
		{
			forceX = (LookAt.x * power / (velocity.x + 0.01f) - LookRight.x * power / (velocity.x + 0.01f)) * 0.707106781186547f;
			forceY = (LookAt.y * power / (velocity.y + 0.01f) - LookRight.y * power / (velocity.y + 0.01f)) * 0.707106781186547f;
			forceZ = (LookAt.z * power / (velocity.z + 0.01f) - LookRight.z * power / (velocity.z + 0.01f)) * 0.707106781186547f;
		}
		else if (moveForwards && moveRight)
		{
			forceX = (LookAt.x * power / (velocity.x + 0.01f) + LookRight.x * power / (velocity.x + 0.01f)) * 0.707106781186547f;
			forceY = (LookAt.y * power / (velocity.y + 0.01f) + LookRight.y * power / (velocity.y + 0.01f)) * 0.707106781186547f;
			forceZ = (LookAt.z * power / (velocity.z + 0.01f) + LookRight.z * power / (velocity.z + 0.01f)) * 0.707106781186547f;
		}
		else if (moveBackwards && moveRight)
		{
			forceX = (-LookAt.x * power / (velocity.x + 0.01f) + LookRight.x * power / (velocity.x + 0.01f)) * 0.707106781186547f;
			forceY = (-LookAt.y * power / (velocity.y + 0.01f) + LookRight.y * power / (velocity.y + 0.01f)) * 0.707106781186547f;
			forceZ = (-LookAt.z * power / (velocity.z + 0.01f) + LookRight.z * power / (velocity.z + 0.01f)) * 0.707106781186547f;
		}
		else if (moveBackwards && moveleft)
		{
			forceX = (-LookAt.x * power / (velocity.x + 0.01f) - LookRight.x * power / (velocity.x + 0.01f)) * 0.707106781186547f;
			forceY = (-LookAt.y * power / (velocity.y + 0.01f) - LookRight.y * power / (velocity.y + 0.01f)) * 0.707106781186547f;
			forceZ = (-LookAt.z * power / (velocity.z + 0.01f) - LookRight.z * power / (velocity.z + 0.01f)) * 0.707106781186547f;
		}
		else if (moveForwards)
		{
			forceX = LookAt.x * power / (velocity.x + 0.01f);
			forceY = LookAt.y * power / (velocity.y + 0.01f);
			forceZ = LookAt.z * power / (velocity.z + 0.01f);
		}
		else if (moveBackwards)
		{
			forceX = -LookAt.x * power / (velocity.x + 0.01f);
			forceY = -LookAt.y * power / (velocity.y + 0.01f);
			forceZ = -LookAt.z * power / (velocity.z + 0.01f);
		}
		else if (moveleft)
		{
			forceX = -LookRight.x * power / (velocity.x + 0.01f);
			forceY = -LookRight.y * power / (velocity.y + 0.01f);
			forceZ = -LookRight.z * power / (velocity.z + 0.01f);
		}
		else if (moveRight)
		{
			forceX = LookRight.x * power / (velocity.x + 0.01f);
			forceY = LookRight.y * power / (velocity.y + 0.01f);
			forceZ = LookRight.z * power / (velocity.z + 0.01f);
		}
		else if (moveUp)
		{
			forceX = LookUp.x * power / (velocity.x + 0.01f);
			forceY = LookUp.y * power / (velocity.y + 0.01f);
			forceZ = LookUp.z * power / (velocity.z + 0.01f);
		}
	}

	auto frameTime = sharedResources.timer.frameTime;
	velocity.x += forceX * oneOverMass * frameTime;
	velocity.y += forceY * oneOverMass * frameTime;
	velocity.z += forceZ * oneOverMass * frameTime;
	location.position.x += velocity.x;
	location.position.y += velocity.y;
	location.position.z += velocity.z;
}