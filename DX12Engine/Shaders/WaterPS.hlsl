// #define USE_REFLECTION_TEXTURE

Texture2D<float4> textures[] : register(t0);
SamplerState SampleType : register(s2);

#include "CameraConstantBuffer.h"
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
	float2 reflectTexCoord = input.position.xy * 0.5 + 0.5f ;
#endif

    float2 normal = (textures[normalTexture].Sample(SampleType, input.texCoords)).xy - 0.5f;

	// Re-position the texture coordinate sampling position by the normal map value to simulate the rippling wave effect.
#ifdef USE_REFLECTION_TEXTURE
	reflectTexCoord += (normal * reflectRefractScale);
#endif

	float2 refractTexCoord;
	refractTexCoord.x = input.position.x / screenWidth + normal.x * reflectRefractScale;
	refractTexCoord.y = input.position.y / screenHeight + normal.y * reflectRefractScale;

    //liniarly interpolate between reflectionTexture and refractionTexture
	float4 color = textures[refractionTexture].Sample(SampleType, refractTexCoord);
	//float4 color = textures[refractionTexture].Sample(SampleType, refractTexCoord);
#ifdef USE_REFLECTION_TEXTURE
    color = lerp(textures[reflectionTexture].Sample(SampleType, reflectTexCoord), color, 0.4f);
#else
	color *= 0.6f;
#endif
	return color;
}