cbuffer VSPerFrameConstantBuffer : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
};

cbuffer VSPerObjectConstantBuffer : register(b1)
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
	float4 refractionPosition : TEXCOORD1;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	input.position.w = 1.0f;
    output.position = mul(mul(viewProjectionMatrix, worldMatrix), input.position);

	output.tex = input.tex;

	output.refractionPosition = output.position;

	return output;
}