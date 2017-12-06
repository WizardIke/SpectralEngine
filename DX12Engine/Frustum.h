#pragma once
#include <directxmath.h>

class Frustum
{
	DirectX::XMVECTOR planes[6];
	//float planes[6][4];
public:
	Frustum() {}
	~Frustum() {}

	void _vectorcall update(DirectX::XMMATRIX ProjectionMatrix, DirectX::XMMATRIX ViewMatrix, float screenNear, float screenDepth);

	bool _vectorcall checkPoint(DirectX::XMVECTOR point) const;
	bool checkCube(float xCenter, float yCenter, float zCenter, float radius) const;
	bool _vectorcall checkSphere(DirectX::XMVECTOR center, float radius) const;
	bool checkCuboid(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize) const;
	bool checkCuboid2(float maxWidth, float maxHeight, float maxDepth, float minWidth, float minHeight, float minDepth) const;
};