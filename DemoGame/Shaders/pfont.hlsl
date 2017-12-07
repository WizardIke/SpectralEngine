Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

cbuffer PSPerObjectBuffer : register(b1)
{
	float4 pixelColor;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float4 color;

	// Sample the texture pixel at this location.
	color = shaderTexture.Sample(SampleType, input.tex);

	// If the color is black on the texture then treat this pixel as transparent.
	if (color.r == 0.0f)
	{
		color.a = 0.0f;
	}
	else
	{
		color.a = 1.0f;
		color = color * pixelColor;
	}

	return color;
}