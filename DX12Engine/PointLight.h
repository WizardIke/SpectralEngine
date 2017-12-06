#pragma once
#include "Vector4.h"

class PointLight
{
public:
	Vector4 position;
	Vector4 baseColor;

	PointLight() {}
	PointLight(Vector4 color, Vector4 position) :
		baseColor(color), position(position) {}
};