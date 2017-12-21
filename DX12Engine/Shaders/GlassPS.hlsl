Texture2D<float4> textures[] : register(t0);
SamplerState clampSampler : register(s1);

#include <GlassMaterialPS.h>

struct Input
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
	// Calculate the projected refraction texture coordinates.
	float2 refractTexCoord;
	refractTexCoord.x = input.position.x * 0.5f + 0.5f;
	refractTexCoord.y = -input.position.y * 0.5f + 0.5f;

	// Sample the normal from the normal map texture.
    float2 normal = textures[normalTexture].Sample(clampSampler, input.tex).xy - 0.5f;

	// Re-position the texture coordinate sampling position by the normal map value to simulate light distortion through glass.
	refractTexCoord = refractTexCoord + (normal.xy * refractionScale);
	float4 color = textures[diffuseTexture].Sample(clampSampler, input.tex);
	float4 refractionColor = textures[refractionTexture].Sample(clampSampler, refractTexCoord);
    color = lerp(refractionColor, color, opacity);

	return color;
}