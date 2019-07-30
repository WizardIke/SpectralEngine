#include "Transform.h"

DirectX::XMMATRIX Transform::toMatrix() const
{
	DirectX::XMVECTOR positionVector = DirectX::XMVectorSet(position.x(), position.y(), position.z(), 0.0f);
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(rotation.x(), rotation.y(), rotation.z());

	DirectX::XMFLOAT3 temp(0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR lookAtVector = DirectX::XMVectorAdd(positionVector, XMVector3TransformCoord(DirectX::XMLoadFloat3(&temp), rotationMatrix));

	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f };
	DirectX::XMVECTOR upVector = XMVector3TransformCoord(DirectX::XMLoadFloat3(&up), rotationMatrix);

	return DirectX::XMMatrixLookAtLH(positionVector, lookAtVector, upVector);
}

Transform Transform::reflection(float height) const
{
	Transform transform;
	transform.position.x() = position.x();
	transform.position.y() = height + height - position.y();
	transform.position.z() = position.z();

	transform.rotation.x() = -rotation.x();
	transform.rotation.y() = rotation.y();
	transform.rotation.z() = rotation.z();
	return transform;
}