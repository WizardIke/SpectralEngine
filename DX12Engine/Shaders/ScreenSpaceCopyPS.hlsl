Texture2D<float4> textures[] : register(t0);
//SamplerState biliniarSampler : register(s2);

cbuffer Material : register(b1)
{
	uint srcTexture;
}

struct Input
{
	float4 position : SV_POSITION;
};

float4 main(Input input) : SV_TARGET
{
	//float2 texCoords = (input.screenPos.xy + 1.f) * 0.5f;
	return textures[srcTexture].Load(int3(input.position.xy, 0));
}