Texture2D<float4> textures[] : register(t0);
#define diffuseTexture 19
SamplerState transparentBorderedSample : register(s0);

struct Input
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

struct Output
{
    float4 color : SV_TARGET;
    float depth : SV_Depth;
};

Output main(Input input)
{
    Output output;
    if (textures[diffuseTexture].Sample(transparentBorderedSample, input.texCoord).a == 0.0f)
    {
        discard;
    }
    output.color = input.color;
    output.depth = 0.0f;
    return output;
}