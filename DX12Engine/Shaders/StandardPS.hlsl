//#define USE_DIRECTIONAL_LIGHT
//#define USE_POINT_LIGHTS

//#define USE_PER_OBJECT_SPECULAR
//#define USE_SPECULAR_TEXTURE

//#define USE_PER_OBJECT_AMBIENT
//#define USE_AMBIENT_TEXTURE

//#define USE_PER_OBJECT_BASE_COLOR
//#define USE_PER_VERTEX_BASE_COLOR
//#define USE_BASE_COLOR_TEXTURE

//#define USE_NORAML_TEXTURE

//#define USE_PER_OBJECT_EMMISIVE_LIGHT
//#define USE_EMMISIVE_TEXTURE

//#define USE_PER_OBJECT_MINREFLECTANCE
//#define USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
//#define USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS

//#define USE_VIRTUAL_TEXTURES

#if defined(USE_SPECULAR_TEXTURE) || defined(USE_AMBIENT_TEXTURE) || defined(USE_BASE_COLOR_TEXTURE) || defined(USE_NORAML_TEXTURE) || defined(USE_EMMISIVE_TEXTURE)
Texture2D<float4> textures[] : register(t0);
SamplerState sampleType : register(s2);
#endif

#if defined(USE_PER_OBJECT_MINREFLECTANCE) || defined(USE_PER_OBJECT_SPECULAR) || defined(USE_SPECULAR_TEXTURE) || defined(USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE)
#include "CameraConstantBuffer.h"
#endif

#include "StandardMaterialPS.h"

#if defined(USE_DIRECTIONAL_LIGHT) || defined(USE_POINT_LIGHTS) || defined(USE_PER_OBJECT_AMBIENT)
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
#endif

struct Input
{
    float4 position : SV_POSITION;
#ifdef USE_PER_VERTEX_BASE_COLOR
    float4 perVertexBaseColor : COLOR;
#endif
#if defined(USE_BASE_COLOR_TEXTURE) || defined(USE_NORAML_TEXTURE) || defined(USE_SPECULAR_TEXTURE) || defined(USE_EMMISIVE_TEXTURE)
    float2 textureCoordinates : TEXCOORD0;
#endif
#if defined(USE_DIRECTIONAL_LIGHT) || defined(USE_POINT_LIGHTS)
    float3 normal : NORMAL;
#ifdef USE_NORAML_TEXTURE
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
#endif
#endif
#if defined(USE_PER_OBJECT_SPECULAR) || defined(USE_SPECULAR_TEXTURE) || defined(USE_POINT_LIGHTS) || defined(USE_PER_OBJECT_MINREFLECTANCE) || defined(USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE)
    float3 worldPosition : POSITION;
#endif
};

#ifdef USE_PER_OBJECT_MINREFLECTANCE
float calulateRefractedLightHighQuality(float nDotL)
{
    return minReflectance + (1 - minReflectance) * pow(nDotL, 5);
}
#endif

#ifdef USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
float3 calulateRefractedLightHighQualityThreeComponents(float nDotL)
{
    return minReflectance + (1 - minReflectance) * pow(nDotL, 5);
}
#endif

#ifdef USE_PER_OBJECT_MINREFLECTANCE
float calulateRefractedLightMediumQuality()
{
    return 1 - minReflectance;
}
#endif

#if defined(USE_PER_OBJECT_MINREFLECTANCE)
float schlickApproximateFresnel(float vDotH)
{
    float base = 1 - vDotH;
    float exponential = pow(base, 5.0);
    return exponential + minReflectance * (1.0 - exponential); //using refractive indices of n1 = 1.45 and n2 = 1, minReflectance = ((n1 - n2) / (n1 + n2))^2 = 0.033735943f
}
#endif

#ifdef USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
float3 schlickApproximateFresnelThreeComponents(float vDotH)
{
    float base = 1 - vDotH;
    float exponential = pow(base, 5.0);
    return exponential + minReflectance * (1.0 - exponential); //using refractive indices of n1 = 1.45 and n2 = 1, minReflectance = ((n1 - n2) / (n1 + n2))^2 = 0.033735943f
}
#endif

float calulateBeckmannDistribution(float nDotH, float roughnessAsMeanQuareSlopeOfMicrofacets)
{
    float nDotHSquared = nDotH * nDotH;
    float value = nDotHSquared * roughnessAsMeanQuareSlopeOfMicrofacets;
    return pow(2.718281828, (nDotHSquared - 1) / value) / (3.141592654 * value * nDotHSquared);
}

float calulateCookTorranceGeometryTermOverBase(float nDotH, float nDotV, float vDotH, float nDotL)
{
    float twoNDotHOverVDotH = 2 * nDotH / vDotH;
    return min(1, min(twoNDotHOverVDotH * nDotV, twoNDotHOverVDotH * nDotL)) /
        (4 * nDotL * nDotV);
}

float calulateImplicitGeometryTermOverBase()
{
    return 0.25f;
}

#ifdef USE_PER_OBJECT_MINREFLECTANCE
float calulateHighQualityBRDFReflection(float3 viewDirection, float3 reflectionNormal, float lightIntensity, float3 normal, float roughnessAsMeanQuareSlopeOfMicrofacets)
{
    float nDotH = dot(normal, reflectionNormal);
    float nDotV = dot(normal, viewDirection);
    float vDotH = dot(viewDirection, reflectionNormal);
    return (schlickApproximateFresnel(nDotH) * calulateBeckmannDistribution(nDotH, roughnessAsMeanQuareSlopeOfMicrofacets) * calulateCookTorranceGeometryTermOverBase(nDotH, nDotV, vDotH, lightIntensity));
}
#endif

#ifdef USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
float3 calulateHighQualityBRDFReflectionThreeComponents(float3 viewDirection, float3 reflectionNormal, float lightIntensity, float3 normal, float roughnessAsMeanQuareSlopeOfMicrofacets)
{
    float nDotH = dot(normal, reflectionNormal);
    float nDotV = dot(normal, viewDirection);
    float vDotH = dot(viewDirection, reflectionNormal);
    return (schlickApproximateFresnelThreeComponents(nDotH) * calulateBeckmannDistribution(nDotH, roughnessAsMeanQuareSlopeOfMicrofacets) * calulateCookTorranceGeometryTermOverBase(nDotH, nDotV, vDotH, lightIntensity));
}
#endif

#if defined(USE_PER_OBJECT_SPECULAR) || defined(USE_SPECULAR_TEXTURE)
float calulateCheapReflection(float3 viewDirection, float3 reflectionNormal, float lightIntensity, float3 normal, float specularPower)
{
    return pow(saturate(dot(reflectionNormal, normal)), specularPower);
}
#endif

float4 sampleTexture(Texture2D<float4> texture, SamplerState samplerType, float2 texCoords)
{
    float4 value;
#ifdef USE_VIRTUAL_TEXTURE
	{
        uint status;
        value = texture.Sample(samplerType, texCoords, status);
        float bias = 1.0;
        while (CheckAccessFullyMapped(status))
        {
            value = texture.SampleBias(samplerType, texCoords, bias, status);
            ++bias;
        }
    }
#else
    value = texture.Sample(samplerType, texCoords);
#endif
    return value;
}

float4 main(Input input) : SV_TARGET
{
#ifdef USE_PER_OBJECT_AMBIENT
    float4 refractedLight = ambientLight;
#elif USE_AMBIENT_TEXTURE
    float4 refractedLight = sampleTexture(textures[ambientTexture], sampleType, input.tex);
#elif USE_DIRECTIONAL_LIGHT || USE_POINT_LIGHTS
    float4 refractedLight = float4(0.0f, 0.0f, 0.0f, 0.0f);
#else
    float4 refractedLight = float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif

#if defined(USE_DIRECTIONAL_LIGHT) || defined(USE_POINT_LIGHTS)
#ifdef USE_NORAML_TEXTURE
	float2 bumpMap = sampleTexture(textures[normalTexture], sampleType, input.textureCoordinates).xy - 0.5f;
    float3 normal = normalize(input.normal) + bumpMap.x * normalize(input.tangent) + bumpMap.y * normalize(input.bitangent);
    normal = normalize(normal);
#else
    float3 normal = normalize(input.normal);
#endif
#endif

#ifdef USE_DIRECTIONAL_LIGHT
#if defined(USE_PER_OBJECT_SPECULAR) || defined(USE_SPECULAR_TEXTURE) || defined(USE_PER_OBJECT_MINREFLECTANCE) || defined(USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE)
    float3 reflectedLight;
#endif
    float lightIntensity = dot(normal, -lightDirection);
    if (lightIntensity > 0.0f)
    {
#if defined(USE_PER_OBJECT_MINREFLECTANCE) || defined(USE_PER_OBJECT_SPECULAR) || defined(USE_SPECULAR_TEXTURE) || defined(USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE)
        float3 viewDirection = normalize(input.worldPosition - cameraPosition);
        float3 reflectionNormal = normalize(viewDirection + lightDirection);
#endif
#if defined(USE_PER_OBJECT_MINREFLECTANCE) && defined(USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS)
        reflectedLight = calulateHighQualityBRDFReflection(viewDirection, reflectionNormal, lightIntensity, normal, roughnessAsMeanQuareSlopeOfMicrofacets);
        refractedLight += calulateRefractedLightHighQuality(lightIntensity);
#elif defined(USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE) && defined(USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS)
        reflectedLight = calulateHighQualityBRDFReflectionThreeComponents(viewDirection, reflectionNormal, lightIntensity, normal, roughnessAsMeanQuareSlopeOfMicrofacets);
        refractedLight.rgb += calulateRefractedLightHighQualityThreeComponents(lightIntensity);
#elif defined(USE_PER_OBJECT_SPECULAR)
        refractedLight += lightIntensity * directionalLight;
        reflectedLight.rgb = specularColor * calulateCheapReflection(viewDirection, reflectionNormal, lightIntensity, normal, specularPower);
#elif defined(USE_SPECULAR_TEXTURE)
		float4 specularMaterial = sampleTexture(textures[specularTexture], sampleType, input.textureCoordinates);
        float2 reflectRefract = calulateCheapReflectionAndRefraction(viewDirection, reflectionNormal, lightIntensity, normal, specularMaterial.a);
        refractedLight += directionalLight * reflectRefract.g;
        reflectedLight.rgb = specularMaterial.rgb * reflectRefract.r;
#else
        refractedLight += (directionalLight * lightIntensity);
#endif

    }
#if defined(USE_PER_OBJECT_SPECULAR) || defined(USE_SPECULAR_TEXTURE) || defined(USE_PER_OBJECT_MINREFLECTANCE) || defined(USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE)
    else
    {
        reflectedLight = float3(0.0f, 0.0f, 0.0f);
    }
#endif
#endif

#ifdef USE_POINT_LIGHTS
    for (uint i = 0u; i < pointLightCount; ++i)
    {
        float3 dispacement = pointLights[i].position.xyz - input.worldPosition;
        float OneOverdistanceSquared = 1 / dot(dispacement, dispacement);
        float lightIntensity = dot(normal, dispacement) * sqrt(OneOverdistanceSquared);
        if (lightIntensity > 0.0f)
        {
            lightIntensity *= OneOverdistanceSquared;
            refractedLight += pointLights[i].baseColor * lightIntensity;
        }
    }
#endif
    
#ifdef USE_PER_OBJECT_BASE_COLOR
    refractedLight *= baseColor;
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    refractedLight.rgb += reflectedLight;
#endif
  
    
#elif defined(USE_PER_VERTEX_BASE_COLOR)
    refractedLight *= perVertexBaseColor;
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    refractedLight.rgb += reflectedLight;
#endif

#elif defined(USE_BASE_COLOR_TEXTURE)
	refractedLight *= sampleTexture(textures[baseColorTexture], sampleType, input.textureCoordinates);
#if defined(USE_PER_OBJECT_SPECULAR) || defined(USE_SPECULAR_TEXTURE) || defined(USE_PER_OBJECT_MINREFLECTANCE) || defined(USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE)
    refractedLight.rgb += reflectedLight;
#endif
#endif

#ifdef USE_PER_OBJECT_EMMISIVE_LIGHT
    refractedLight += emmisiveColor;
#endif
#ifdef USE_EMMISIVE_TEXTURE
	refractedLight += sampleTexture(textures[emmisiveTexture], sampleType, input.textureCoordinates);
#endif

    return saturate(refractedLight);
}