Texture2D<float4> textures[] : register(t0);
SamplerState sampleType;

cbuffer perObjectBuffer : register(b1)
{
    float4 baseColor;
    uint baseColorTexture;
};

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float fogFactor : FOG;
};

float4 main(Input input) : SV_TARGET
{
    float4 color = textures[baseColorTexture].Sample(sampleType, input.tex);
    color = input.fogFactor * (color - baseColor) + baseColor;
    return color;
}