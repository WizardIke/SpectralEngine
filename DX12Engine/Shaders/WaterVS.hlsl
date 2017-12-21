cbuffer Camera : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
};

#include <WaterMaterialVS.h>

struct Input
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
};

struct Output
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

Output main(Input input)
{
	Output output;

	input.position.w = 1.0f;
	float4 worldPosition = mul(worldMatrix, input.position);
    output.position = mul(viewProjectionMatrix, worldPosition);

	output.tex = input.tex + waterTranslation;
	return output;
}