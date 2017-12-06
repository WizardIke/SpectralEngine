Texture2D<float4> textures[] : register(t0);
SamplerState sampleType;

cbuffer perObjectBuffer : register(b1)
{
    uint texture1;
    uint texture2;
    uint blendTexture;
}

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
    float4 color1 = textures[texture1].Sample(sampleType, input.tex);
    float4 color2 = textures[texture2].Sample(sampleType, input.tex);
    float4 blendValue = textures[blendTexture].Sample(sampleType, input.tex);

	// Combine the two textures based on the alpha value.
    float4 color = blendValue * (color1 - color2) + color2;
    color = saturate(color);

    return color;
}