Texture2D<float4> textures[] : register(t0);

SamplerState SampleType : register(s2);

cbuffer PSPerObjectConstantBuffer : register(b1)
{
    float4 specularColor;
    float specularPower;
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

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 viewDirection : TEXCOORD1;
};

float4 main(PixelInputType input) : SV_TARGET
{
    float4 light = ambientLight;
    float3 normal = normalize(input.normal);
    float lightIntensity = dot(normal, -lightDirection);
    if (lightIntensity > 0.0f)
    {
        light += (directionalLight * lightIntensity);

        float3 reflection = normalize(2.0f * lightIntensity * normal + lightDirection);
        light += pow(max(dot(reflection, normalize(input.viewDirection)), 0.0f), specularPower); //add specular
    }

    float4 color = textures[diffuseTexture].Sample(SampleType, input.tex) * light;
    color = saturate(color);

    return color;
}