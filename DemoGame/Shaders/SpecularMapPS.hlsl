Texture2D<float4> textures[] : register(t0);
SamplerState sampleType;

cbuffer LightBuffer : register(b1)
{
    float4 ambientColor;
    float4 diffuseColor;
    float4 specularColor;
    float3 lightDirection;
    uint baseColorTexture;
    uint normalTexture;
    uint specularTexture;
};

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    float3 viewDirection : TEXCOORD1;
};

float4 main(Input input) : SV_TARGET
{
    float4 color = textures[baseColorTexture].Sample(sampleType, input.tex);

    float2 bumpMap = textures[normalTexture].Sample(sampleType, input.tex).xy - 0.5f;

    float3 normal = normalize(input.normal);
    float3 bumpNormal = normal + bumpMap.x * normalize(input.tangent) + bumpMap.y * (input.binormal);
    bumpNormal = normalize(bumpNormal);

    float lightIntensity = dot(bumpNormal, -lightDirection);

    float4 light = ambientColor;

    if (lightIntensity > 0.0f)
    {
        light += (diffuseColor * lightIntensity);

        float4 specularIntensity = textures[specularTexture].Sample(sampleType, input.tex);
        float3 reflection = 2.0f * lightIntensity * bumpNormal + lightDirection;
        light.rgb += specularIntensity.rgb * pow(max(normalize(dot(reflection, input.viewDirection)), 0.0f), specularIntensity.a); //add specular
    }

    color *= light;

    return color;
}