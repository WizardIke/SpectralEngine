Texture2D<float4> textures[] : register(t0);
SamplerState transparentBorderedSample : register(s0);

cbuffer textures : register(b1)
{
    uint diffuseTexture;
};

struct Input
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texCoords : TEXCOORD;
};

float4 main(Input input) : SV_TARGET
{
    return input.color * textures[diffuseTexture].Sample(transparentBorderedSample, input.texCoords);
}