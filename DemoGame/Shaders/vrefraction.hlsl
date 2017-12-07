#pragma pack_matrix( row_major )

cbuffer VSPerFrameConstantBuffer : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
	float float_1;
};

cbuffer VSPerObjectConstantBuffer : register(b1)
{
	matrix worldMatrix;
	float4 clipPlane;
};

struct VertexInputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float clip : SV_ClipDistance0;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	input.position.w = 1.0f;

	output.position = mul(input.position, mul(worldMatrix, viewProjectionMatrix));

	output.tex = input.tex;

	// Calculate the normal vector against the world matrix only.
	output.normal = mul(input.normal, (float3x3)worldMatrix);
	output.normal = normalize(output.normal);
	// Set the clipping plane.
	output.clip = dot(mul(input.position, worldMatrix), clipPlane);

	return output;
}