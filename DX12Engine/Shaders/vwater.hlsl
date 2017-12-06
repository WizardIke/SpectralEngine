cbuffer VSPerFrameConstantBuffer : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
};

cbuffer VSPerObjectConstantBuffer : register(b1)
{
	matrix worldViewProjectionMatrix;
	matrix worldReflectionProjectionMatrix;
	float waterTranslation;
};

struct VertexInput
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 reflectionPosition : TEXCOORD1;
	float4 refractionPosition : TEXCOORD2;
};

PixelInput main(VertexInput input)
{
	PixelInput output;

	input.position.w = 1.0f;
    output.position = mul(worldViewProjectionMatrix, input.position);

	output.tex = input.tex + waterTranslation;

    output.reflectionPosition = mul(worldReflectionProjectionMatrix, input.position);
	output.refractionPosition = output.position;

	return output;
}