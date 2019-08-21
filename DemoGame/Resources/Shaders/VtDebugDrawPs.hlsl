Texture2D<float4> textures[] : register(t0);
SamplerState sampleType : register(s2);

cbuffer material : register(b1)
{
	uint baseColorTexture;
};

struct Input
{
    float4 position : SV_POSITION;
    float2 textureCoordinates : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
    float4 color = textures[baseColorTexture].Sample(sampleType, input.textureCoordinates);
    color.r = color.r * 4000.0f + color.a / 4.0f;
    color.g *= 4000.0f;
    color.b *= 30.0f;
    color.a = 1.0f;
    return color;
}