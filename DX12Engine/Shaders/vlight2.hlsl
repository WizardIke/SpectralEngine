cbuffer VSPerFrameBuffer : register(b0)
{
	matrix viewProjectionMatrix;
	float3 camerapos;
}

cbuffer VSPerObjectConstantBuffer : register(b1)
{
	matrix worldMatrix;
	float4 lightPosition[4];
}

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
	float3 lightPos1 : TEXCOORD1;
	float3 lightPos2 : TEXCOORD2;
	float3 lightPos3 : TEXCOORD3;
	float3 lightPos4 : TEXCOORD4;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	float4 worldPosition;

	input.position.w = 1.0f;
    worldPosition = mul(worldMatrix, input.position);
    output.position = mul(viewProjectionMatrix, worldPosition);

	output.tex = input.tex;

    output.normal = mul((float3x3)worldMatrix, input.normal);
	output.normal = normalize(output.normal);

	output.lightPos1.xyz = lightPosition[0].xyz - worldPosition.xyz;
	output.lightPos2.xyz = lightPosition[1].xyz - worldPosition.xyz;
	output.lightPos3.xyz = lightPosition[2].xyz - worldPosition.xyz;
	output.lightPos4.xyz = lightPosition[3].xyz - worldPosition.xyz;

	output.lightPos1 = normalize(output.lightPos1);
	output.lightPos2 = normalize(output.lightPos2);
	output.lightPos3 = normalize(output.lightPos3);
	output.lightPos4 = normalize(output.lightPos4);

	return output;
}