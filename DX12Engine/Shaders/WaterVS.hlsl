cbuffer Camera : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
};

#include <WaterMaterialVS.h>

struct Input
{
	float3 position : POSITION;
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

	float4 worldPosition = mul(worldMatrix, float4(input.position, 1.0f));
    output.position = mul(viewProjectionMatrix, worldPosition);

	output.tex = input.tex + waterTranslation;
	return output;
}