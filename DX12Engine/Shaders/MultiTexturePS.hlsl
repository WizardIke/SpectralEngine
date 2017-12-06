Texture2D<float4> textures[] : register(t0);
SamplerState sampleType;

cbuffer PSPerObjectBuffer : register(b1)
{
    uint baseColorTexture1;
    uint baseColorTexture2;
}

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
    float4 color1 = textures[baseColorTexture1].Sample(sampleType, input.tex);
    float4 color2 = textures[baseColorTexture2].Sample(sampleType, input.tex);

	// Blend the two pixels together and multiply by the gamma value.
    float4 color = color1 * color2 * 2.0;
    color = saturate(color);

    return color;
}