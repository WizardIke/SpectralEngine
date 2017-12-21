cbuffer Camera : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
};

cbuffer Material : register(b1)
{
	matrix worldMatrix;
};

struct VertexInputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	input.position.w = 1.0f;
    output.position = mul(mul(viewProjectionMatrix, worldMatrix), input.position);

	output.tex = input.tex;

	return output;
}