Texture2D shaderTexture;
SamplerState SampleType;

cbuffer TransparentBuffer
{
	float blendAmount;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float4 color;

	color = shaderTexture.Sample(SampleType, input.tex);

	color.a = blendAmount;

	return color;
}