Texture2D<float4> textures[] : register(t0);
SamplerState wrapSampler : register(s2);

cbuffer PSPerObjectConstantBuffer : register(b1)
{
	float refractionScale;
    uint refractionTexture;
    uint DiffuseTexture;
    uint NormalTexture;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 refractionPosition : TEXCOORD1;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float2 refractTexCoord;
	float4 refractionColor;

	// Calculate the projected refraction texture coordinates.
	refractTexCoord.x = input.refractionPosition.x / input.refractionPosition.w / 2.0f + 0.5f;
	refractTexCoord.y = -input.refractionPosition.y / input.refractionPosition.w / 2.0f + 0.5f;

	// Sample the normal from the normal map texture.
    float4 normalMap = textures[NormalTexture].Sample(wrapSampler, input.tex);

    float2 normal = normalMap.xy - 0.5f;

	// Re-position the texture coordinate sampling position by the normal map value to simulate light distortion through glass.
	refractTexCoord = refractTexCoord + (normal.xy * refractionScale);

	// Sample the texture pixel from the refraction texture using the perturbed texture coordinates.
    refractionColor = textures[refractionTexture].Sample(wrapSampler, refractTexCoord);

	// Sample the texture pixel from the glass color texture.
    float4 textureColor = textures[DiffuseTexture].Sample(wrapSampler, input.tex);

	// Evenly combine the glass color and refraction value for the final color.
    float4 color = lerp(refractionColor, textureColor, 0.5f);

	return color;
}