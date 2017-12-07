Texture2D<float4> textures[] : register(t0);
SamplerState sampleType;

cbuffer FadeBuffer : register(b1)
{
    float fadeAmount;
    uint baseColorTexture;
};

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
    float4 color;

    color = textures[baseColorTexture].Sample(sampleType, input.tex);
    color = color * fadeAmount;

    return color;
}