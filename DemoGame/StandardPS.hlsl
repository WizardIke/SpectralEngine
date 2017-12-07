#define USE_DIRECTIONAL_LIGHT 1
#define USE_POINT_LIGHTS 1

//#define USE_PER_OBJECT_SPECULAR 1
//#define USE_SPECULAR_TEXTURE 1

#define USE_PER_OBJECT_AMBIENT 1
//#define USE_AMBIENT_TEXTURE 1  // used for global illumination

//#define USE_PER_OBJECT_BASE_COLOR 1
//#define USE_PER_VERTEX_BASE_COLOR 1
#define USE_BASE_COLOR_TEXTURE 1

//#define USE_NORAML_TEXTURE 1

//#define USE_PER_OBJECT_EMMISIVE_LIGHT 1
//#define USE_EMMISIVE_TEXTURE 1

//#define USE_PER_OBJECT_MINREFLECTANCE 1
#define USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE 1
#define USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS 1

#if USE_SPECULAR_TEXTURE || USE_AMBIENT_TEXTURE || USE_BASE_COLOR_TEXTURE || USE_NORAML_TEXTURE || USE_EMMISIVE_TEXTURE
Texture2D<float4> textures[] : register(t0);
SamplerState sampleType : register(s2);
#endif

#if USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
cbuffer camera : register(b0)
{
    matrix viewProjectionMatrix;
    float3 cameraPosition;
}
#endif

cbuffer material : register(b1)
{
#if USE_PER_OBJECT_BASE_COLOR
    float4 baseColor;
#endif
#if USE_PER_OBJECT_SPECULAR
    float3 specularColor;
    float specularPower;
#endif
#if USE_PER_OBJECT_EMMISIVE_LIGHT
    float4 emmisiveColor;
#endif

#if USE_PER_OBJECT_MINREFLECTANCE
    float minReflectance;
#endif

#if USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    float3 minReflectance;
#endif

#if USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS
    float roughnessAsMeanQuareSlopeOfMicrofacets;
#endif

#if USE_BASE_COLOR_TEXTURE
    uint baseColorTexture;
#endif

#if USE_SPECULAR_TEXTURE
    uint specularTexture;
#endif

#ifdef USE_AMBIENT_TEXTURE
     uint ambientTexture;
#endif

#if USE_NORAML_TEXTURE
    uint normalTexture;
#endif

#if USE_EMMISIVE_TEXTURE
    uint emmisiveTexture;
#endif
};

#if USE_DIRECTIONAL_LIGHT || USE_POINT_LIGHTS || USE_PER_OBJECT_AMBIENT
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
#ifdef USE_BASE_COLOR_TEXTURE
    float2 textureCoordinates : TEXCOORD0;
#endif
#if USE_DIRECTIONAL_LIGHT || USE_POINT_LIGHTS
    float3 normal : NORMAL;
#if USE_NORAML_TEXTURE
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
#endif
#endif
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_POINT_LIGHTS || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    float3 worldPosition : POSITION;
#endif
};

#if USE_PER_OBJECT_MINREFLECTANCE
float calulateRefractedLightHighQuality(in float nDotL)
{
    return minReflectance + (1 - minReflectance) * pow(nDotL, 5);
}
#endif

#if USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
float3 calulateRefractedLightHighQualityThreeComponents(in float nDotL)
{
    return minReflectance + (1 - minReflectance) * pow(nDotL, 5);
}
#endif

#if USE_PER_OBJECT_MINREFLECTANCE
float calulateRefractedLightMediumQuality()
{
    return 1 - minReflectance;
}
#endif

#if USE_PER_OBJECT_MINREFLECTANCE
float schlickApproximateFresnel(in float vDotH)
{
    float base = 1 - vDotH;
    float exponential = pow(base, 5.0);
    return exponential + minReflectance * (1.0 - exponential); //using refractive indices of n1 = 1.45 and n2 = 1, minReflectance = ((n1 - n2) / (n1 + n2))^2 = 0.033735943f
}
#endif

#if USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
float3 schlickApproximateFresnelThreeComponents(in float vDotH)
{
    float base = 1 - vDotH;
    float exponential = pow(base, 5.0);
    return exponential + minReflectance * (1.0 - exponential); //using refractive indices of n1 = 1.45 and n2 = 1, minReflectance = ((n1 - n2) / (n1 + n2))^2 = 0.033735943f
}
#endif

float calulateBeckmannDistribution(in float nDotH, in float roughnessAsMeanQuareSlopeOfMicrofacets)
{
    float nDotHSquared = nDotH * nDotH;
    float value = nDotHSquared * roughnessAsMeanQuareSlopeOfMicrofacets;
    return pow(2.718281828, (nDotHSquared - 1) / value) / (3.141592654 * value * nDotHSquared);
}

float calulateCookTorranceGeometryTermOverBase(in float nDotH, in float nDotV, in float vDotH, in float nDotL)
{
    float twoNDotHOverVDotH = 2 * nDotH / vDotH;
    return min(1, min(twoNDotHOverVDotH * nDotV, twoNDotHOverVDotH * nDotL)) /
        (4 * nDotL * nDotV);
}

float calulateImplicitGeometryTermOverBase()
{
    return 0.25f;
}

#if USE_PER_OBJECT_MINREFLECTANCE
float calulateHighQualityBRDFReflection(in float3 viewDirection, in float3 reflectionNormal, in float lightIntensity, in float3 normal, in float roughnessAsMeanQuareSlopeOfMicrofacets)
{
    float nDotH = dot(normal, reflectionNormal);
    float nDotV = dot(normal, viewDirection);
    float vDotH = dot(viewDirection, reflectionNormal);
    return (schlickApproximateFresnel(nDotH) * calulateBeckmannDistribution(nDotH, roughnessAsMeanQuareSlopeOfMicrofacets) * calulateCookTorranceGeometryTermOverBase(nDotH, nDotV, vDotH, lightIntensity));
}
#endif

#if USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
float3 calulateHighQualityBRDFReflectionThreeComponents(in float3 viewDirection, in float3 reflectionNormal, in float lightIntensity, in float3 normal, in float roughnessAsMeanQuareSlopeOfMicrofacets)
{
    float nDotH = dot(normal, reflectionNormal);
    float nDotV = dot(normal, viewDirection);
    float vDotH = dot(viewDirection, reflectionNormal);
    return (schlickApproximateFresnelThreeComponents(nDotH) * calulateBeckmannDistribution(nDotH, roughnessAsMeanQuareSlopeOfMicrofacets) * calulateCookTorranceGeometryTermOverBase(nDotH, nDotV, vDotH, lightIntensity));
}
#endif

#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE
float2 calulateCheapReflectionAndRefraction(in float3 viewDirection, in float3 reflectionNormal, in float lightIntensity, in float3 normal, in float specularPower)
{
    float fresnel = schlickApproximateFresnel(dot(viewDirection, reflectionNormal));
    return float2(pow(saturate(dot(reflectionNormal, normal)), specularPower), lightIntensity * (1 - fresnel));
}
#endif

float4 main(Input input) : SV_TARGET
{
#if USE_PER_OBJECT_AMBIENT
    float4 refractedLight = ambientLight;
#elif USE_AMBIENT_TEXTURE
    float4 refractedLight = textures[ambientTexture].Sample(sampleType, input.tex);
#elif USE_DIRECTIONAL_LIGHT || USE_POINT_LIGHTS
    float4 refractedLight = float4(0.0f, 0.0f, 0.0f, 0.0f);
#else
    float4 refractedLight = float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif

#if USE_DIRECTIONAL_LIGHT || USE_POINT_LIGHTS
#ifdef USE_NORAML_TEXTURE
    float2 bumpMap = textures[normalTexture].Sample(sampleType, input.textureCoordinates).xy - 0.5f;
    float3 normal = normalize(input.normal) + bumpMap.x * normalize(input.tangent) + bumpMap.y * normalize(input.bitangent);
    normal = normalize(normal);
#else
    float3 normal = normalize(input.normal);
#endif
#endif

#if USE_DIRECTIONAL_LIGHT
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    float3 reflectedLight;
#endif
    float lightIntensity = dot(normal, -lightDirection);
    if (lightIntensity > 0.0f)
    {
#if USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
        float3 viewDirection = normalize(input.worldPosition - cameraPosition);
        float3 reflectionNormal = normalize(viewDirection + lightDirection);
#endif
#if USE_PER_OBJECT_MINREFLECTANCE && USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS
        reflectedLight = calulateHighQualityBRDFReflection(viewDirection, reflectionNormal, lightIntensity, normal, roughnessAsMeanQuareSlopeOfMicrofacets);
        refractedLight += calulateRefractedLightHighQuality(lightIntensity);
#elif USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE && USE_PER_OBJECT_ROUGHNESS_AS_MEAN_QUARED_SLOPE_OF_MICROFACETS
        reflectedLight = calulateHighQualityBRDFReflectionThreeComponents(viewDirection, reflectionNormal, lightIntensity, normal, roughnessAsMeanQuareSlopeOfMicrofacets);
        refractedLight.rgb += calulateRefractedLightHighQualityThreeComponents(lightIntensity);
#elif USE_PER_OBJECT_SPECULAR
        float2 reflectRefract = calulateCheapReflectionAndRefraction(viewDirection, reflectionNormal, lightIntensity, normal, specularPower);
        refractedLight += directionalLight * reflectRefract.g;
        reflectedLight.rgb = specularColor * reflectRefract.r;
#elif USE_SPECULAR_TEXTURE
        float4 specularMaterial = textures[specularTexture].Sample(sampleType, input.textureCoordinates);
        float2 reflectRefract = calulateCheapReflectionAndRefraction(viewDirection, reflectionNormal, lightIntensity, normal, specularMaterial.a);
        refractedLight += directionalLight * reflectRefract.g;
        reflectedLight.rgb = specularMaterial.rgb * reflectRefract.r;
#else
        refractedLight += (directionalLight * lightIntensity);
#endif

    }
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
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
    
#if USE_PER_OBJECT_BASE_COLOR
    refractedLight *= baseColor;
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    refractedLight.rgb += reflectedLight;
#endif
  
    
#elif USE_PER_VERTEX_BASE_COLOR
    refractedLight *= perVertexBaseColor;
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    refractedLight.rgb += reflectedLight;
#endif

#elif USE_BASE_COLOR_TEXTURE
    refractedLight *= textures[baseColorTexture].Sample(sampleType, input.textureCoordinates);
#if USE_PER_OBJECT_SPECULAR || USE_SPECULAR_TEXTURE || USE_PER_OBJECT_MINREFLECTANCE || USE_PER_OBJECT_THREE_COMPONENT_MINREFLECTANCE
    refractedLight.rgb += reflectedLight;
#endif
#endif

#ifdef USE_PER_OBJECT_EMMISIVE_LIGHT
    refractedLight += emmisiveColor;
#endif
#ifdef USE_EMMISIVE_TEXTURE
    refractedLight += textures[emmisiveTexture].Sample(sampleType, input.textureCoordinates);
#endif

    return saturate(refractedLight);
}