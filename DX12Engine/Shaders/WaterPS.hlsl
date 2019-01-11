// #define USE_REFLECTION_TEXTURE

Texture2D<float4> textures[] : register(t0);
SamplerState clampSample : register(s1);
SamplerState wrapSample : register(s2);

#include "CameraConstantBuffer.h"
#include "WaterMaterialPS.h"

struct Input
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
};

float4 main(Input input) : SV_TARGET
{
    float2 normal = (textures[normalTexture].Sample(wrapSample, input.texCoords)).xy - 0.5f;

	float3 viewVector = cameraPosition - input.worldPos;
	float oneOverDistanceToCamera = rsqrt(dot(viewVector, viewVector));

	float2 refractTexCoord;
	refractTexCoord.x = input.position.x / screenWidth + normal.x * reflectRefractScale * oneOverDistanceToCamera;
	refractTexCoord.y = input.position.y / screenHeight + normal.y * reflectRefractScale * oneOverDistanceToCamera;

    //liniarly interpolate between reflectionTexture and refractionTexture
	float4 color = textures[refractionTexture].Sample(clampSample, refractTexCoord);
#ifdef USE_REFLECTION_TEXTURE
    color = lerp(textures[reflectionTexture].Sample(clampSample, float2(refractTexCoord.x, 1 - refractTexCoord.y)), color, 0.4f);
#else
	color *= 0.6f;
#endif
	return color;
}