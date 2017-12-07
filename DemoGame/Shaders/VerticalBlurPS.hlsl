/* used with TextureVS */

Texture2D<float4> textures[] : register(t0);
SamplerState clampSample;

cbuffer PerObjectBuffer : register(b1)
{
    uint baseColorTexture;
    float pixelHeight;
}

struct Input
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
    input.tex.y -= pixelHeight * 4.0f;
    float4 color = 0.0278 * textures[baseColorTexture].Sample(clampSample, input.tex);
    input.tex.y += pixelHeight;
    color += 0.0656 * textures[baseColorTexture].Sample(clampSample, input.tex);
    input.tex.y += pixelHeight;
    color += 0.1210 * textures[baseColorTexture].Sample(clampSample, input.tex);
    input.tex.y += pixelHeight;
    color += 0.1747 * textures[baseColorTexture].Sample(clampSample, input.tex);

    input.tex.y += pixelHeight;
    color += 0.1974 * textures[baseColorTexture].Sample(clampSample, input.tex);
    input.tex.y += pixelHeight;

    color += 0.1747 * textures[baseColorTexture].Sample(clampSample, input.tex);
    input.tex.y += pixelHeight;
    color += 0.1210 * textures[baseColorTexture].Sample(clampSample, input.tex);
    input.tex.y += pixelHeight;
    color += 0.0656 * textures[baseColorTexture].Sample(clampSample, input.tex);
    input.tex.y += pixelHeight;
    color += 0.0278 * textures[baseColorTexture].Sample(clampSample, input.tex);

    return color;
}