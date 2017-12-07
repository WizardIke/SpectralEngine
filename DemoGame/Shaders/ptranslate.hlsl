Texture2D shaderTexture;
SamplerState SampleType;

cbuffer TranslationBuffer
{
	float textureTranslation;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PixelInputType input) : SV_TARGET
{
	// Translate the position where we sample the pixel from.
	input.tex.x += textureTranslation;

return shaderTexture.Sample(SampleType, input.tex);
}