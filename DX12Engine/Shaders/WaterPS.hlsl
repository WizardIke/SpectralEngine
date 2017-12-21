// #define USE_REFLECTION_TEXTURE

Texture2D<float4> textures[] : register(t0);
SamplerState SampleType : register(s2);

#include "WaterMaterialPS.h"

struct Input
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
#ifdef USE_REFLECTION_TEXTURE
	// Calculate the projected reflection texture coordinates.
	float2 reflectTexCoord;
	reflectTexCoord.x = input.position.x * 0.5f + 0.5f;
	reflectTexCoord.y = 0.5f + input.position.y * 0.5f;
#endif

	// Calculate the projected refraction texture coordinates.
	float2 refractTexCoord;
	refractTexCoord.x = input.position.x * 0.5f + 0.5f;
	refractTexCoord.y = -input.position.y * 0.5f + 0.5f;

    float2 normal = (textures[normalTexture].Sample(SampleType, input.texCoords)).xy - 0.5f;

	// Re-position the texture coordinate sampling position by the normal map value to simulate the rippling wave effect.
#ifdef USE_REFLECTION_TEXTURE
	reflectTexCoord += (normal * reflectRefractScale);
#endif
	refractTexCoord += (normal * reflectRefractScale);

    //liniarly interpolate between reflectionTexture and refractionTexture
#ifdef USE_REFLECTION_TEXTURE
    float4 color = lerp(textures[reflectionTexture].Sample(SampleType, reflectTexCoord), textures[refractionTexture].Sample(SampleType, refractTexCoord), 0.4f);
#else
	float4 color = textures[refractionTexture].Sample(SampleType, refractTexCoord) * 0.6f;
#endif
	return color;
}