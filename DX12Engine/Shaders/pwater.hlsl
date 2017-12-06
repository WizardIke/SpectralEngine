SamplerState SampleType : register(s2);

Texture2D<float4> textures[] : register(t0);
#define normalTexture 12

cbuffer PSPerObjectConstantBuffer : register(b1)
{
	float reflectRefractScale;
    uint reflectionTexture;
    uint refractionTexture;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 reflectionPosition : TEXCOORD1;
	float4 refractionPosition : TEXCOORD2;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float2 reflectTexCoord;
	float2 refractTexCoord;

	// Calculate the projected reflection texture coordinates.
	reflectTexCoord.x = input.reflectionPosition.x / input.reflectionPosition.w * 0.5f + 0.5f;
	reflectTexCoord.y = -input.reflectionPosition.y / input.reflectionPosition.w * 0.5f + 0.5f;

	// Calculate the projected refraction texture coordinates.
	refractTexCoord.x = input.refractionPosition.x / input.refractionPosition.w * 0.5f + 0.5f;
	refractTexCoord.y = -input.refractionPosition.y / input.refractionPosition.w * 0.5f + 0.5f;

    float3 normal = (textures[normalTexture].Sample(SampleType, input.tex)).xyz - 0.5f;

	// Re-position the texture coordinate sampling position by the normal map value to simulate the rippling wave effect.
	reflectTexCoord = reflectTexCoord + (normal.xy * reflectRefractScale);
	refractTexCoord = refractTexCoord + (normal.xy * reflectRefractScale);

    //liniarly interpolate between reflectionTexture and refractionTexture
    float4 color = textures[reflectionTexture].Sample(SampleType, reflectTexCoord);
    color += 0.4f * (textures[refractionTexture].Sample(SampleType, refractTexCoord) - color);
	
   return color;
}