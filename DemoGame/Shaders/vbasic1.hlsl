#pragma pack_matrix( row_major )

cbuffer VSPerObjectBuffer : register(b1)
{
	matrix worldViewProjection;
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
	output.position = mul(input.position, worldViewProjection);
	output.tex = input.tex;

	return output;
}