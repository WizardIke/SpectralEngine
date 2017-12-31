Texture2D<float4> textures[] : register(t0);
SamplerState clampSampler : register(s1);

#include "CameraConstantBuffer.h"
#include "GlassMaterialPS.h"

struct Input
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 worldPosition : POSITION;
};

float4 main(Input input) : SV_TARGET
{
	// Sample the normal from the normal map texture.
    float2 normal = textures[normalTexture].Sample(clampSampler, input.tex).xy - 0.5f;

	float3 viewVector = cameraPosition - input.worldPosition;
	float oneOverDistanceToCamera = 1 / length(viewVector);

	// Re-position the texture coordinate sampling position by the normal map value to simulate light distortion through glass.
	float2 refractTexCoord;
	refractTexCoord.x = input.position.x / screenWidth + normal.x * refractionScale * oneOverDistanceToCamera;
	refractTexCoord.y = input.position.y / screenHeight + normal.y * refractionScale * oneOverDistanceToCamera;

	float4 color = textures[diffuseTexture].Sample(clampSampler, input.tex);
	float4 refractionColor = textures[refractionTexture].Sample(clampSampler, refractTexCoord);
    color = lerp(refractionColor, color, opacity);

	return color;
}