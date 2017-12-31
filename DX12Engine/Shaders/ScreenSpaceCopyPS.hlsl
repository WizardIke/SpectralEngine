Texture2D<float4> textures[] : register(t0);

#include "CameraConstantBuffer.h"

struct Input
{
	float4 position : SV_POSITION;
};

float4 main(Input input) : SV_TARGET
{
	return textures[backBufferTexture].Load(int3(input.position.xy, 0));
}