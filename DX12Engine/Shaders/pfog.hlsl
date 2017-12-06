Texture2D shaderTexture;
SamplerState SampleType;

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float fogFactor : FOG;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float4 textureColor;
	float4 fogColor;
	float4 finalColor;

	// Sample the texture pixel at this location.
	textureColor = shaderTexture.Sample(SampleType, input.tex);

	// Set the color of the fog to grey.
	fogColor = float4(0.5f, 0.5f, 0.5f, 1.0f);

	// Calculate the final color using the fog effect equation.
	finalColor = input.fogFactor * textureColor + (1.0 - input.fogFactor) * fogColor;

	return finalColor;
}