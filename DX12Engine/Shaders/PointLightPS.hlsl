Texture2D<float4> textures[] : register(t0);
SamplerState sampleType : register(s2);

cbuffer camera : register(b0)
{
    matrix viewProjectionMatrix;
    float3 cameraPosition;
}

cbuffer perObjectConstantBuffer : register(b1)
{
    uint diffuseTexture;
};

struct PointLight
{
    float4 position;
    float4 baseColor;
};

cbuffer lights : register(b2)
{
    float4 ambientLight;
    float4 directionalLight;
    float3 lightDirection;
    uint pointLightCount;
    PointLight pointLights[14u];
}

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION;
};

float4 main(Input input) : SV_TARGET
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (uint i = 0u; i < pointLightCount; ++i)
    {
        float3 dispacement = pointLights[i].position.xyz - input.worldPosition;
        float lightIntensity = normalize(dot(input.normal, dispacement));
        if (lightIntensity > 0.0f)
        {
            lightIntensity /= dot(dispacement, dispacement);
            color += pointLights[i].baseColor * lightIntensity;
        }
    }

    color *= textures[diffuseTexture].Sample(sampleType, input.tex);
    color = saturate(color);

    return color;
}