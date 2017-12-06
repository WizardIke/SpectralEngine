#include "Frustum.h"

void Frustum::update(DirectX::XMMATRIX projectionMatrix, DirectX::XMMATRIX viewMatrix, float screenNear, float screenDepth)
{
	DirectX::XMFLOAT4X4 pMatrix, matrix;
	DirectX::XMMATRIX finalMatrix;

	// Convert the projection matrix into a 4x4 float type.
	XMStoreFloat4x4(&pMatrix, projectionMatrix);
	
	// Calculate the minimum Z distance in the frustum.
	float r = screenDepth / (screenDepth - screenNear);

	// Load the updated values back into the projection matrix.
	pMatrix._33 = r;
	pMatrix._43 = -r * screenNear;
	projectionMatrix = XMLoadFloat4x4(&pMatrix);

	// Create the frustum matrix from the view matrix and updated projection matrix.
	finalMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);

	// Convert the final matrix into a 4x4 float type.
	XMStoreFloat4x4(&matrix, finalMatrix);

	// Calculate near plane of frustum.
	planes[0u] = DirectX::XMVectorSet(matrix._14 + matrix._13, matrix._24 + matrix._23, matrix._34 + matrix._33, matrix._44 + matrix._43);
	// Normalize the near plane.
	planes[0u] = DirectX::XMPlaneNormalize(planes[0u]);

	// Calculate far plane of frustum.
	planes[1u] = DirectX::XMVectorSet(matrix._14 - matrix._13, matrix._24 - matrix._23, matrix._34 - matrix._33, matrix._44 - matrix._43);
	// Normalize the far plane.
	planes[1u] = DirectX::XMPlaneNormalize(planes[1u]);

	// Calculate left plane of frustum.
	planes[2u] = DirectX::XMVectorSet(matrix._14 + matrix._11, matrix._24 + matrix._21, matrix._34 + matrix._31, matrix._44 + matrix._41);
	// Normalize the left plane.
	planes[2u] = DirectX::XMPlaneNormalize(planes[2u]);

	// Calculate right plane of frustum.
	planes[3u] = DirectX::XMVectorSet(matrix._14 - matrix._11, matrix._24 - matrix._21, matrix._34 - matrix._31, matrix._44 - matrix._41);
	// Normalize the right plane.
	planes[3u] = DirectX::XMPlaneNormalize(planes[3u]);

	// Calculate top plane of frustum.
	planes[4u] = DirectX::XMVectorSet(matrix._14 - matrix._12, matrix._24 - matrix._22, matrix._34 - matrix._32, matrix._44 - matrix._42);
	// Normalize the top plane.
	planes[4u] = DirectX::XMPlaneNormalize(planes[4u]);

	// Calculate bottom plane of frustum.
	planes[5u] = DirectX::XMVectorSet(matrix._14 + matrix._12, matrix._24 + matrix._22, matrix._34 + matrix._32, matrix._44 + matrix._42);
	// Normalize the bottom plane.
	planes[5u] = DirectX::XMPlaneNormalize(planes[5u]);
}


bool _vectorcall Frustum::checkPoint(DirectX::XMVECTOR point) const
{
	// Check each of the six planes to make sure the point is inside all of them and hence inside the frustum.
	for (auto i = 0u; i < 6u; ++i)
	{
		// Calculate the dot product of the plane and the 3D point.
		DirectX::XMFLOAT4 dotProduct;
		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(point, planes[i]));

		// Determine if the point is on the correct side of the current plane, exit out if it is not.
		if (dotProduct.x <= 0.0f) return false;
	}

	return true;
}


bool Frustum::checkCube(float xCenter, float yCenter, float zCenter, float radius) const
{
	DirectX::XMFLOAT4 dotProduct;

	// Check each of the six planes to see if the cube is inside the frustum.
	for (auto i = 0u; i < 6u; ++i)
	{
		// Check all eight points of the cube to see if they all reside within the frustum.
		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - radius, yCenter - radius, zCenter - radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + radius, yCenter - radius, zCenter - radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - radius, yCenter + radius, zCenter - radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - radius, yCenter - radius, zCenter + radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + radius, yCenter + radius, zCenter - radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + radius, yCenter - radius, zCenter + radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - radius, yCenter + radius, zCenter + radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + radius, yCenter + radius, zCenter + radius, 1.0f), planes[i]));
		if (dotProduct.x > 0.0f)
		{
			continue;
		}

		return false;
	}

	return true;
}



bool _vectorcall Frustum::checkSphere(DirectX::XMVECTOR center, float radius) const
{
	DirectX::XMFLOAT4 dotProduct;

	// Check the six planes to see if the sphere is inside them or not.
	for (auto i = 0u; i < 6u; ++i)
	{
		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(center, planes[i]));
		if (dotProduct.x <= -radius)
		{
			return false;
		}
	}

	return true;
}



bool Frustum::checkCuboid(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize) const
{
	DirectX::XMFLOAT4 dotProduct;

	// Check each of the six planes to see if the rectangle is in the frustum or not.
	for (auto i = 0u; i < 6u; ++i)
	{
		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - xSize, yCenter - ySize, zCenter - zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + xSize, yCenter - ySize, zCenter - zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - xSize, yCenter + ySize, zCenter - zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + xSize, yCenter + ySize, zCenter - zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - xSize, yCenter - ySize, zCenter + zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + xSize, yCenter - ySize, zCenter + zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter - xSize, yCenter + ySize, zCenter + zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(xCenter + xSize, yCenter + ySize, zCenter + zSize, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		return false;
	}

	return true;
}

bool Frustum::checkCuboid2(float maxWidth, float maxHeight, float maxDepth, float minWidth, float minHeight, float minDepth) const
{
	DirectX::XMFLOAT4 dotProduct;
	
	// Check if any of the 6 planes of the rectangle are inside the view frustum.
	for (auto i = 0u; i < 6u; ++i)
	{
		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(minWidth, minHeight, minDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(maxWidth, minHeight, minDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(minWidth, maxHeight, minDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(maxWidth, maxHeight, minDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(minWidth, minHeight, maxDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(maxWidth, minHeight, maxDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(minWidth, maxHeight, maxDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		DirectX::XMStoreFloat4(&dotProduct, DirectX::XMVector4Dot(DirectX::XMVectorSet(maxWidth, maxHeight, maxDepth, 1.0f), planes[i]));
		if (dotProduct.x >= 0.0f)
		{
			continue;
		}

		return false;
	}

	return true;
}