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
	float3 worldPosition : TEXCOORD1;
};

Output main(Input input)
{
	Output output;


	float4 worldPos = mul(worldMatrix, float4(input.position, 1.0f));
    output.position = mul(viewProjectionMatrix, worldPos);
	output.worldPosition = worldPos.xyz;

	output.tex = input.tex + waterTranslation;
	return output;
}