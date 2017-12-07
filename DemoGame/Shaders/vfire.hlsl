cbuffer VSPerFrameBuffer : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
}

cbuffer VSPerObjectBuffer : register(b1)
{
	matrix worldMatrix;
    float flameTime;
	float3 scrollSpeeds;
	float3 scales;
}

struct VertexInputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float2 texCoords1 : TEXCOORD1;
	float2 texCoords2 : TEXCOORD2;
	float2 texCoords3 : TEXCOORD3;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	input.position.w = 1.0f;

    output.position = mul(mul(viewProjectionMatrix, worldMatrix), input.position);

	output.tex = input.tex;

	// Compute texture coordinates for first noise texture using the first scale and upward scrolling speed values.
	output.texCoords1 = (input.tex * scales.x);
	output.texCoords1.y = output.texCoords1.y + (flameTime * scrollSpeeds.x);

	// Compute texture coordinates for second noise texture using the second scale and upward scrolling speed values.
	output.texCoords2 = (input.tex * scales.y);
	output.texCoords2.y = output.texCoords2.y + (flameTime * scrollSpeeds.y);

	// Compute texture coordinates for third noise texture using the third scale and upward scrolling speed values.
	output.texCoords3 = (input.tex * scales.z);
	output.texCoords3.y = output.texCoords3.y + (flameTime * scrollSpeeds.z);

	return output;
}
