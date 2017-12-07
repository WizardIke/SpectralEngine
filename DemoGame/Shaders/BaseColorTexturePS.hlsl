Texture2D<float4> textures[] : register(t0);
SamplerState sampleType;

cbuffer PerObjectBuffer : register(b1)
{
    uint baseColorTexture;
}

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(PixelInputType input) : SV_TARGET
{
    float4 color = textures[baseColorTexture].Sample(sampleType, input.tex);
    return color;
}