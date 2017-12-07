Texture2D<float4> textures[] : register(t0);
SamplerState sampleType;

cbuffer LightBuffer : register(b1)
{
    float3 lightDirection;
    uint baseColorTexture;
    uint bumpTexture;
};

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
};

float4 main(Input input) : SV_TARGET
{
    float4 color = textures[baseColorTexture].Sample(sampleType, input.tex);
    float4 bumpMap = textures[bumpTexture].Sample(sampleType, input.tex);

    bumpMap = bumpMap - 0.5f;

    float3 bumpNormal = (bumpMap.x * input.tangent) + (bumpMap.y * input.bitangent) + (bumpMap.z * input.normal);
    bumpNormal = normalize(bumpNormal);

    float lightIntensity = dot(bumpNormal, -lightDirection);

    color = saturate(lightIntensity * color);

    return color;
}