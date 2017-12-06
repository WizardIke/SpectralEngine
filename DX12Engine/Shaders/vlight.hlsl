cbuffer VSPerFrameConstantBuffer : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
};

cbuffer VSPerObjectConstantBuffer : register(b1)
{
	matrix worldMatrix;
};

struct Input
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL0;
};

struct Output
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDirection : TEXCOORD1;
};

Output main(Input input)
{
    Output output;

	input.position.w = 1.0f;
    float4 worldPosition = mul(worldMatrix, input.position);
    output.position = mul(viewProjectionMatrix, worldPosition);

	output.tex = input.tex;
    output.normal = mul((float3x3) worldMatrix, input.normal);
    output.viewDirection = cameraPosition.xyz - worldPosition.xyz;

	return output;
}