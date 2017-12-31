Texture2D<float4> textures[] : register(t0);

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
	return textures[srcTexture].Load(int3(input.position.xy, 0));
}